#pragma once

#include <string>
#include "base/files/file_path.h"
#include "base/no_destructor.h"

namespace neovex {

class NeovexAutoUpdater {
public:
    static NeovexAutoUpdater* GetInstance();
    
    // Download installer silently to temp dir
    void StartDownload(const std::string& url);
    
    // Called on browser exit - runs installer silently
    void ApplyUpdateIfReady();
    
    // Returns true if installer is downloaded and ready
    bool IsUpdateReady() const;

private:
    friend class base::NoDestructor<NeovexAutoUpdater>;

    NeovexAutoUpdater() = default;
    ~NeovexAutoUpdater() = default;

    void OnDownloadComplete(const base::FilePath& path);

    base::FilePath downloaded_installer_path_;
    bool update_ready_ = false;
};

} // namespace neovex
