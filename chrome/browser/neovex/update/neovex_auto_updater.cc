#include "chrome/browser/neovex/update/neovex_auto_updater.h"

#include "base/files/file_util.h"
#include "base/path_service.h"
#include "base/task/thread_pool.h"
#include "base/functional/bind.h"
#include "base/command_line.h"
#include "base/process/launch.h"
#include "base/threading/thread_restrictions.h"
#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/lifetime/application_lifetime.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "components/prefs/pref_service.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace neovex {

// NEOVEX AUTOUPDATE

class NeovexAutoUpdateGlobalError : public GlobalErrorWithStandardBubble {
public:
    explicit NeovexAutoUpdateGlobalError(const std::string& message) : message_(message) {}

    bool HasMenuItem() override { return false; }
    int MenuItemCommandID() override { return 0; }
    std::u16string MenuItemLabel() override { return std::u16string(); }
    void ExecuteMenuItem(Browser* browser) override {}
    
    std::u16string GetBubbleViewTitle() override { return u"Neovex Update"; }
    std::vector<std::u16string> GetBubbleViewMessages() override { return {base::UTF8ToUTF16(message_)}; }
    std::u16string GetBubbleViewAcceptButtonLabel() override { return u"OK"; }
    std::u16string GetBubbleViewCancelButtonLabel() override { return std::u16string(); }
    
    void OnBubbleViewDidClose(Browser* browser) override {}
    void BubbleViewAcceptButtonPressed(Browser* browser) override {}
    void BubbleViewCancelButtonPressed(Browser* browser) override {}

    base::WeakPtr<GlobalErrorWithStandardBubble> AsWeakPtr() override {
        return weak_ptr_factory_.GetWeakPtr();
    }

private:
    std::string message_;
    base::WeakPtrFactory<NeovexAutoUpdateGlobalError> weak_ptr_factory_{this};
};

static void ShowUpdateNotification(const std::string& message) {
    Browser* browser = chrome::FindLastActive();
    if (!browser) return;
    
    Profile* profile = browser->profile();
    GlobalErrorService* error_service = GlobalErrorServiceFactory::GetForProfile(profile);
    if (error_service) {
        auto error = std::make_unique<NeovexAutoUpdateGlobalError>(message);
        error->ShowBubbleView(browser);
        error_service->AddGlobalError(std::move(error));
    }
}

NeovexAutoUpdater* NeovexAutoUpdater::GetInstance() {
    static base::NoDestructor<NeovexAutoUpdater> instance;
    return instance.get();
}

void NeovexAutoUpdater::StartDownload(const std::string& url) {
    // Check if we already have a ready update from a previous session
    std::string saved_path = g_browser_process->local_state()->GetString("neovex.update_installer_path");
    if (!saved_path.empty()) {
        base::FilePath path = base::FilePath::FromUTF8Unsafe(saved_path);
        
        base::ThreadPool::PostTaskAndReplyWithResult(
            FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
            base::BindOnce([](base::FilePath p) { return base::PathExists(p); }, path),
            base::BindOnce([](base::FilePath p, bool exists) {
                if (exists) {
                    NeovexAutoUpdater::GetInstance()->update_ready_ = true;
                    NeovexAutoUpdater::GetInstance()->downloaded_installer_path_ = p;
                    ShowUpdateNotification("Update ready. Restart Neovex to apply.");
                } else {
                    g_browser_process->local_state()->SetString("neovex.update_installer_path", "");
                }
            }, path)
        );
        return;
    }

    ShowUpdateNotification("Downloading Neovex update...");

    auto resource_request = std::make_unique<network::ResourceRequest>();
    resource_request->url = GURL(url);
    
    net::NetworkTrafficAnnotationTag traffic_annotation =
        net::DefineNetworkTrafficAnnotation("neovex_auto_updater", R"(
          semantics {
            sender: "Neovex Auto Updater"
            description: "Downloads the latest Neovex installer."
            trigger: "Triggered automatically when a new version is detected."
            data: "None."
            destination: OTHER
          }
          policy {
            cookies_allowed: NO
            setting: "This feature cannot be disabled by settings."
            policy_exception_justification: "Not implemented."
          }
        )");

    auto url_loader = network::SimpleURLLoader::Create(std::move(resource_request), traffic_annotation);
    
    base::ThreadPool::PostTaskAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::BEST_EFFORT},
        base::BindOnce([]() {
            base::FilePath temp_dir;
            base::GetTempDir(&temp_dir);
            base::FilePath dest_path = temp_dir.AppendASCII("neovex_update").AppendASCII("mini_installer.exe");
            base::CreateDirectory(dest_path.DirName());
            return dest_path;
        }),
        base::BindOnce([](std::unique_ptr<network::SimpleURLLoader> loader,
                          const base::FilePath& path) {
            auto* raw_loader = loader.get();
            raw_loader->DownloadToFile(
                g_browser_process->system_network_context_manager()->GetSharedURLLoaderFactory().get(),
                base::BindOnce([](std::unique_ptr<network::SimpleURLLoader> loader,
                                  base::FilePath downloaded_path) {
                    if (!downloaded_path.empty()) {
                        NeovexAutoUpdater::GetInstance()->OnDownloadComplete(downloaded_path);
                    }
                }, std::move(loader)),
                path);
        }, std::move(url_loader))
    );
}

void NeovexAutoUpdater::OnDownloadComplete(const base::FilePath& path) {
    downloaded_installer_path_ = path;
    update_ready_ = true;

    // Save installer path to local prefs so it survives browser restart
    g_browser_process->local_state()->SetString("neovex.update_installer_path", path.AsUTF8Unsafe());

    ShowUpdateNotification("Update ready. Restart Neovex to apply.");
}

bool NeovexAutoUpdater::IsUpdateReady() const {
    return update_ready_;
}

void NeovexAutoUpdater::ApplyUpdateIfReady() {
    if (!update_ready_ || downloaded_installer_path_.empty()) return;

    base::ThreadPool::PostTask(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE, base::TaskShutdownBehavior::BLOCK_SHUTDOWN},
        base::BindOnce([](base::FilePath path) {
            if (base::PathExists(path)) {
                base::CommandLine cmd(path);
                cmd.AppendSwitch("--silent");
                cmd.AppendSwitch("--system-level");
                base::LaunchOptions options;
                options.wait = false;
                base::LaunchProcess(cmd, options);
            }
        }, downloaded_installer_path_)
    );

    // Clear the path so it doesn't run again
    g_browser_process->local_state()->SetString("neovex.update_installer_path", "");

    chrome::AttemptRestart();
}

} // namespace neovex
