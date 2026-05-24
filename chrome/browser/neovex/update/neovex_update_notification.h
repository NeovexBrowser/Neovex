#pragma once
#include <string>

namespace neovex {

// NEOVEX UPDATE
class NeovexUpdateNotification {
public:
    static void Show(const std::string& version,
                     const std::string& download_url,
                     const std::string& release_notes);
};

} // namespace neovex
