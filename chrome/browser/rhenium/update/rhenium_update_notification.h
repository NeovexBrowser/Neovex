#pragma once
#include <string>

namespace rhenium {

// RHENIUM UPDATE
class RheniumUpdateNotification {
public:
    static void Show(const std::string& version,
                     const std::string& download_url,
                     const std::string& release_notes);
};

} // namespace rhenium
