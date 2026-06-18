// Copyright 2025 Neovex Authors. All rights reserved.
// Study Mode Navigation Throttle - blocks distracting websites

#ifndef CHROME_BROWSER_STUDY_MODE_STUDY_MODE_NAVIGATION_THROTTLE_H_
#define CHROME_BROWSER_STUDY_MODE_STUDY_MODE_NAVIGATION_THROTTLE_H_

#include <set>
#include <string>
#include <vector>

#include "content/public/browser/navigation_throttle.h"
#include "content/public/browser/navigation_throttle_registry.h"

class StudyModeNavigationThrottle : public content::NavigationThrottle {
 public:
  using ThrottleCheckResult = content::NavigationThrottle::ThrottleCheckResult;

  explicit StudyModeNavigationThrottle(
      content::NavigationThrottleRegistry& registry);
  ~StudyModeNavigationThrottle() override;

  StudyModeNavigationThrottle(const StudyModeNavigationThrottle&) = delete;
  StudyModeNavigationThrottle& operator=(const StudyModeNavigationThrottle&) =
      delete;

  const char* GetNameForLogging() override;
  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;

  static void MaybeCreateAndAdd(
      content::NavigationThrottleRegistry& registry);

  // Called from the NTP StudyModeMessageHandler
  static void SetEnabled(bool enabled);
  static void SetWhitelist(const std::vector<std::string>& domains);

  static bool IsEnabled();

 private:
  ThrottleCheckResult CheckNavigation();
  static bool IsDomainBlocked(const std::string& host);
};

// --- Neovex Shield Privacy Service ---

#include <atomic>
#include "base/no_destructor.h"

class NeovexShieldService {
 public:
  static NeovexShieldService* GetInstance();
  
  void IncrementTrackersBlocked();
  void IncrementAdsBlocked();
  void IncrementFingerprintsBlocked();
  void IncrementCookiesManaged();

  int GetTrackersBlocked() const;
  int GetAdsBlocked() const;
  int GetFingerprintsBlocked() const;
  int GetCookiesManaged() const;

 private:
  friend class base::NoDestructor<NeovexShieldService>;
  NeovexShieldService();
  ~NeovexShieldService() = default;

  std::atomic<int> trackers_blocked_{0};
  std::atomic<int> ads_blocked_{0};
  std::atomic<int> fingerprints_blocked_{0};
  std::atomic<int> cookies_managed_{0};
};

class NeovexShieldThrottle : public content::NavigationThrottle {
 public:
  static void MaybeCreateAndAdd(content::NavigationThrottleRegistry& registry);

  explicit NeovexShieldThrottle(content::NavigationThrottleRegistry& registry);
  ~NeovexShieldThrottle() override;

  ThrottleCheckResult WillStartRequest() override;
  ThrottleCheckResult WillRedirectRequest() override;
  const char* GetNameForLogging() override;

 private:
  ThrottleCheckResult CheckIfBlocked();
};

#include "third_party/blink/public/common/loader/url_loader_throttle.h"

// Throttle for subresources
class NeovexShieldURLLoaderThrottle : public blink::URLLoaderThrottle {
 public:
  NeovexShieldURLLoaderThrottle() = default;
  ~NeovexShieldURLLoaderThrottle() override = default;

  void WillStartRequest(network::ResourceRequest* request, bool* defer) override;
  void WillRedirectRequest(
      net::RedirectInfo* redirect_info,
      const network::mojom::URLResponseHead& response_head,
      bool* defer,
      std::vector<std::string>* to_be_removed_request_headers,
      net::HttpRequestHeaders* modified_request_headers,
      net::HttpRequestHeaders* modified_cors_exempt_request_headers) override;
};

#endif  // CHROME_BROWSER_STUDY_MODE_STUDY_MODE_NAVIGATION_THROTTLE_H_
