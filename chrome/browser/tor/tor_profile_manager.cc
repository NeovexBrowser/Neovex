// Copyright 2024 Neovex Authors. All rights reserved.
// NEOVEX TOR

#include "chrome/browser/tor/tor_profile_manager.h"

#include "base/logging.h"
#include "base/values.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/proxy_config_pref_names.h"

namespace neovex {

// static
void TorProfileManager::ApplyTorProxy(Profile* profile) {
  if (!profile) {
    return;
  }

  LOG(INFO) << "NEOVEX TOR: Applying SOCKS5 proxy 127.0.0.1:9050 to profile";

  PrefService* prefs = profile->GetPrefs();
  if (!prefs) {
    LOG(ERROR) << "NEOVEX TOR: No pref service for profile";
    return;
  }

  // Build the proxy config dictionary manually.
  base::DictValue proxy_dict;
  proxy_dict.Set("mode", "fixed_servers");
  proxy_dict.Set("server", "socks5://127.0.0.1:9050");
  proxy_dict.Set("bypass_list", "");

  // PrefService::Set takes const base::Value&, construct one from DictValue.
  prefs->Set(proxy_config::prefs::kProxy,
             base::Value(std::move(proxy_dict)));
}

}  // namespace neovex
