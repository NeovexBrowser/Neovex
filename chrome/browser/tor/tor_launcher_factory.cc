// Copyright 2024 Neovex Authors. All rights reserved.
// NEOVEX TOR

#include "chrome/browser/tor/tor_launcher_factory.h"

#include "base/logging.h"
#include "base/no_destructor.h"

namespace neovex {

// static
TorLauncherFactory* TorLauncherFactory::GetInstance() {
  static base::NoDestructor<TorLauncherFactory> instance;
  return instance.get();
}

TorLauncherFactory::TorLauncherFactory() = default;
TorLauncherFactory::~TorLauncherFactory() = default;

void TorLauncherFactory::OnIncognitoWindowOpened() {
  incognito_window_count_++;
  LOG(INFO) << "NEOVEX TOR: Incognito window opened (count: "
            << incognito_window_count_ << ")";
}

void TorLauncherFactory::EnableTor() {
  if (!launcher_) {
    launcher_ = std::make_unique<TorLauncher>();
    launcher_->Start();
  } else if (!launcher_->IsRunning()) {
    launcher_->Start();
  }
}

void TorLauncherFactory::DisableTor() {
  if (launcher_) {
    launcher_->Stop();
    launcher_.reset();
  }
}

void TorLauncherFactory::OnIncognitoWindowClosed() {
  if (incognito_window_count_ > 0) {
    incognito_window_count_--;
  }
  LOG(INFO) << "NEOVEX TOR: Incognito window closed (count: "
            << incognito_window_count_ << ")";

  if (incognito_window_count_ == 0 && launcher_) {
    LOG(INFO) << "NEOVEX TOR: Last incognito window closed, stopping Tor";
    launcher_->Stop();
    launcher_.reset();
  }
}

TorLauncher* TorLauncherFactory::GetLauncher() {
  return launcher_.get();
}

}  // namespace neovex
