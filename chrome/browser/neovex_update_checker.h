// Copyright 2026 Neovex Authors
// Background update checker that uses GitHub Releases API.

#ifndef CHROME_BROWSER_NEOVEX_UPDATE_CHECKER_H_
#define CHROME_BROWSER_NEOVEX_UPDATE_CHECKER_H_

#include <optional>
#include <memory>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class PrefService;

namespace neovex {

// Pref names for the Neovex update system.
extern const char kUpdateDownloaded[];
extern const char kUpdateInstallerPath[];
extern const char kUpdateLastVersion[];

// Checks GitHub Releases for a newer Neovex version, downloads the
// mini_installer.exe asset in the background, and sets local-state prefs
// when the download is ready.  All network I/O uses SimpleURLLoader on the
// UI thread; the file write is dispatched to base::ThreadPool.
class UpdateChecker {
 public:
  UpdateChecker(PrefService* local_state,
                scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~UpdateChecker();

  UpdateChecker(const UpdateChecker&) = delete;
  UpdateChecker& operator=(const UpdateChecker&) = delete;

  // Kicks off the version check.  Safe to call from the UI thread.
  void CheckForUpdate();

 private:
  void OnReleaseFetched(std::optional<std::string> body);
  void OnInstallerDownloaded(base::FilePath path);

  raw_ptr<PrefService> local_state_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<network::SimpleURLLoader> loader_;
  std::unique_ptr<network::SimpleURLLoader> download_loader_;
  std::string download_url_;

  base::WeakPtrFactory<UpdateChecker> weak_factory_{this};
};

// Launch the downloaded installer as a detached process.
// Called from the shutdown path.  Never blocks.
void MaybeLaunchInstallerOnShutdown(PrefService* local_state);

}  // namespace neovex

#endif  // CHROME_BROWSER_NEOVEX_UPDATE_CHECKER_H_
