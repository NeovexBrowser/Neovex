// Copyright 2024 Rhenium Authors. All rights reserved.
// RHENIUM TOR

#ifndef CHROME_BROWSER_TOR_TOR_PROFILE_MANAGER_H_
#define CHROME_BROWSER_TOR_TOR_PROFILE_MANAGER_H_

class Profile;

namespace rhenium {

// TorProfileManager applies Tor SOCKS5 proxy settings to incognito profiles.
class TorProfileManager {
 public:
  // Apply the Tor SOCKS5 proxy configuration to the given profile.
  static void ApplyTorProxy(Profile* profile);

  // Remove the Tor SOCKS5 proxy configuration from the given profile.
  static void RemoveTorProxy(Profile* profile);
};

}  // namespace rhenium

#endif  // CHROME_BROWSER_TOR_TOR_PROFILE_MANAGER_H_
