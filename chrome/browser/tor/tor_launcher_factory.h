// Copyright 2024 Neovex Authors. All rights reserved.
// NEOVEX TOR

#ifndef CHROME_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
#define CHROME_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_

#include <memory>

#include "base/no_destructor.h"
#include "chrome/browser/tor/tor_launcher.h"

namespace neovex {

// TorLauncherFactory is a process-wide singleton that manages the lifecycle
// of the Tor SOCKS5 proxy. It starts Tor when the first incognito window
// is opened and stops it when the last incognito window closes.
class TorLauncherFactory {
 public:
  static TorLauncherFactory* GetInstance();

  TorLauncherFactory(const TorLauncherFactory&) = delete;
  TorLauncherFactory& operator=(const TorLauncherFactory&) = delete;

  // Called when an incognito window is opened.
  void OnIncognitoWindowOpened();

  // Called when an incognito window is closed.
  void OnIncognitoWindowClosed();

  // Returns the TorLauncher instance (may be null if not started).
  TorLauncher* GetLauncher();

 private:
  friend class base::NoDestructor<TorLauncherFactory>;

  TorLauncherFactory();
  ~TorLauncherFactory();

  std::unique_ptr<TorLauncher> launcher_;
  int incognito_window_count_ = 0;
};

}  // namespace neovex

#endif  // CHROME_BROWSER_TOR_TOR_LAUNCHER_FACTORY_H_
