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

#endif  // CHROME_BROWSER_STUDY_MODE_STUDY_MODE_NAVIGATION_THROTTLE_H_
