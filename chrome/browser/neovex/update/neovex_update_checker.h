#pragma once

#include <string>
#include "base/functional/callback.h"
#include "base/no_destructor.h"

namespace neovex {

// NEOVEX UPDATE
struct UpdateInfo {
    UpdateInfo();
    ~UpdateInfo();
    UpdateInfo(const UpdateInfo&);
    UpdateInfo& operator=(const UpdateInfo&);
    
    std::string latest_version;
    std::string installer_url;
    std::string release_notes;
    bool update_available = false;
};

class NeovexUpdateChecker {
public:
    static NeovexUpdateChecker* GetInstance();
    
    // Check once per day silently on startup
    void CheckForUpdates();
    
    // Returns current installed version from chrome/VERSION
    std::string GetCurrentVersion();
    
    // Compare semver strings
    bool IsNewerVersion(const std::string& remote,
                        const std::string& local);
    
    UpdateInfo GetLastCheckResult() const;

private:
    friend class base::NoDestructor<NeovexUpdateChecker>;

    NeovexUpdateChecker() = default;
    ~NeovexUpdateChecker() = default;

    void OnVersionFetched(const std::string& json_response);
    
    UpdateInfo last_result_;
};

} // namespace neovex
