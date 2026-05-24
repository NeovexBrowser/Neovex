#include "chrome/browser/neovex/update/neovex_update_checker.h"
#include "chrome/browser/neovex/update/neovex_auto_updater.h"

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/values.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/neovex/update/neovex_update_notification.h"
#include "chrome/browser/net/system_network_context_manager.h"
#include "chrome/common/channel_info.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace neovex {

// NEOVEX UPDATE

UpdateInfo::UpdateInfo() = default;
UpdateInfo::~UpdateInfo() = default;
UpdateInfo::UpdateInfo(const UpdateInfo&) = default;
UpdateInfo& UpdateInfo::operator=(const UpdateInfo&) = default;

namespace {
const char kUpdateUrl[] = "https://github.com/pahal-desai/Neovex/releases/latest/download/version.json";
const char kLastUpdateCheckPref[] = "neovex.last_update_check_time";
}

NeovexUpdateChecker* NeovexUpdateChecker::GetInstance() {
    static base::NoDestructor<NeovexUpdateChecker> instance;
    return instance.get();
}

void NeovexUpdateChecker::CheckForUpdates() {
    // Run in background / avoid blocking startup
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostDelayedTask(
        FROM_HERE,
        base::BindOnce([]() {
            PrefService* local_state = g_browser_process->local_state();
            if (!local_state) return;

            base::Time last_check = local_state->GetTime(kLastUpdateCheckPref);
            base::Time now = base::Time::Now();
            if (now - last_check < base::Days(1)) {
                return; // Checked within last 24 hours
            }

            local_state->SetTime(kLastUpdateCheckPref, now);

            auto request = std::make_unique<network::ResourceRequest>();
            request->url = GURL(kUpdateUrl);

            net::NetworkTrafficAnnotationTag traffic_annotation =
                net::DefineNetworkTrafficAnnotation("neovex_update_checker", R"(
                semantics {
                    sender: "Neovex Update Checker"
                    description: "Checks for Neovex browser updates."
                    trigger: "Once per day on browser startup."
                    data: "None."
                    destination: OTHER
                }
                policy {
                    cookies_allowed: NO
                    setting: "This feature cannot be disabled."
                })");

            auto url_loader = network::SimpleURLLoader::Create(std::move(request), traffic_annotation);
            
            auto* system_network_context = g_browser_process->system_network_context_manager();
            if (!system_network_context) return;
            
            auto url_loader_factory = system_network_context->GetSharedURLLoaderFactory();

            auto* raw_loader = url_loader.get();
            raw_loader->DownloadToString(
                url_loader_factory.get(),
                base::BindOnce([](std::unique_ptr<network::SimpleURLLoader> loader,
                                  std::optional<std::string> response_body) {
                    if (response_body && loader->NetError() == net::OK) {
                        NeovexUpdateChecker::GetInstance()->OnVersionFetched(*response_body);
                    }
                }, std::move(url_loader)),
                1024 * 1024 /* 1 MB max */);
        }),
        base::Seconds(5)); // Delay check by 5s to not impact startup
}

std::string NeovexUpdateChecker::GetCurrentVersion() {
    return std::string(version_info::GetVersionNumber());
}

bool NeovexUpdateChecker::IsNewerVersion(const std::string& remote, const std::string& local) {
    std::vector<std::string> remote_parts = base::SplitString(remote, ".", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);
    std::vector<std::string> local_parts = base::SplitString(local, ".", base::KEEP_WHITESPACE, base::SPLIT_WANT_ALL);

    for (size_t i = 0; i < std::max(remote_parts.size(), local_parts.size()); ++i) {
        int remote_val = 0;
        int local_val = 0;

        if (i < remote_parts.size()) {
            base::StringToInt(remote_parts[i], &remote_val);
        }
        if (i < local_parts.size()) {
            base::StringToInt(local_parts[i], &local_val);
        }

        if (remote_val > local_val) return true;
        if (remote_val < local_val) return false;
    }
    return false;
}

void NeovexUpdateChecker::OnVersionFetched(const std::string& json_response) {
    std::optional<base::DictValue> dict = base::JSONReader::ReadDict(json_response, 0); // 0 = JSON_PARSE_RFC
    if (!dict) return;

    const std::string* version = dict->FindString("version");
    const std::string* installer_url = dict->FindString("installer_url");
    const std::string* release_notes = dict->FindString("release_notes");

    if (version && installer_url && release_notes) {
        last_result_.latest_version = *version;
        last_result_.installer_url = *installer_url;
        last_result_.release_notes = *release_notes;
        
        std::string current_version = GetCurrentVersion();
        if (IsNewerVersion(*version, current_version)) {
            last_result_.update_available = true;
            // NEOVEX AUTOUPDATE
            neovex::NeovexAutoUpdater::GetInstance()->StartDownload(*installer_url);
        }
    }
}

UpdateInfo NeovexUpdateChecker::GetLastCheckResult() const {
    return last_result_;
}

} // namespace neovex
