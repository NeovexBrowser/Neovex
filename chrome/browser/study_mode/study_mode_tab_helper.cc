// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/study_mode/study_mode_tab_helper.h"
#include "chrome/browser/study_mode/study_mode_navigation_throttle.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"
#include "third_party/blink/public/mojom/loader/resource_load_info.mojom.h"

StudyModeTabHelper::StudyModeTabHelper(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents),
      content::WebContentsUserData<StudyModeTabHelper>(*web_contents) {}

void StudyModeTabHelper::ResourceLoadComplete(
    content::RenderFrameHost* render_frame_host,
    const content::GlobalRequestID& request_id,
    const blink::mojom::ResourceLoadInfo& resource_load_info) {
  if (!resource_load_info.original_url.SchemeIsHTTPOrHTTPS()) {
    return;
  }

  std::string host = std::string(resource_load_info.original_url.host());
  if (NeovexShieldService::IsTrackerDomainForShield(host)) {
    auto* shield = NeovexShieldService::GetInstance();
    shield->IncrementTrackersBlocked();

    if (host.find("doubleclick") != std::string::npos ||
        host.find("syndication") != std::string::npos ||
        host.find("ad") != std::string::npos) {
      shield->IncrementAdsBlocked();
    }

    if (host.find("crwdcntrl") != std::string::npos ||
        host.find("demdex") != std::string::npos) {
      shield->IncrementFingerprintsBlocked();
    }
  }
}

WEB_CONTENTS_USER_DATA_KEY_IMPL(StudyModeTabHelper);
