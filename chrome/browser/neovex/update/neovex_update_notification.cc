#include "chrome/browser/neovex/update/neovex_update_notification.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/global_error/global_error.h"
#include "chrome/browser/ui/global_error/global_error_service.h"
#include "chrome/browser/ui/global_error/global_error_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "ui/base/page_transition_types.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"
#include "content/public/browser/page_navigator.h"

namespace neovex {

// NEOVEX UPDATE

class NeovexUpdateGlobalError : public GlobalErrorWithStandardBubble {
public:
    NeovexUpdateGlobalError(const std::string& version, const std::string& download_url)
        : version_(version), download_url_(download_url) {}

    bool HasMenuItem() override { return false; } // We just show the bubble
    int MenuItemCommandID() override { return 0; }
    std::u16string MenuItemLabel() override { return std::u16string(); }
    void ExecuteMenuItem(Browser* browser) override {}
    
    std::u16string GetBubbleViewTitle() override {
        return u"Neovex Update Available";
    }
    std::vector<std::u16string> GetBubbleViewMessages() override {
        return {base::UTF8ToUTF16("A new version of Neovex (" + version_ + ") is available to download.")};
    }
    std::u16string GetBubbleViewAcceptButtonLabel() override {
        return u"Download Update";
    }
    std::u16string GetBubbleViewCancelButtonLabel() override {
        return u"Not Now";
    }
    
    void OnBubbleViewDidClose(Browser* browser) override {}
    
    void BubbleViewAcceptButtonPressed(Browser* browser) override {
        content::OpenURLParams params(
            GURL(download_url_), content::Referrer(), WindowOpenDisposition::NEW_FOREGROUND_TAB,
            ui::PAGE_TRANSITION_LINK, false);
        browser->OpenURL(params, /*navigation_handle_callback=*/{});
    }
    void BubbleViewCancelButtonPressed(Browser* browser) override {}

    base::WeakPtr<GlobalErrorWithStandardBubble> AsWeakPtr() override {
        return weak_ptr_factory_.GetWeakPtr();
    }

private:
    std::string version_;
    std::string download_url_;
    base::WeakPtrFactory<NeovexUpdateGlobalError> weak_ptr_factory_{this};
};

void NeovexUpdateNotification::Show(const std::string& version,
                                    const std::string& download_url,
                                    const std::string& release_notes) {
    Browser* browser = chrome::FindLastActive();
    if (!browser) return;
    
    Profile* profile = browser->profile();
    GlobalErrorService* error_service = GlobalErrorServiceFactory::GetForProfile(profile);
    if (error_service) {
        auto error = std::make_unique<NeovexUpdateGlobalError>(version, download_url);
        // We show the bubble directly
        error->ShowBubbleView(browser);
        error_service->AddGlobalError(std::move(error));
    }
}

} // namespace neovex
