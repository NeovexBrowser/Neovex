// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_STUDY_MODE_STUDY_MODE_TAB_HELPER_H_
#define CHROME_BROWSER_STUDY_MODE_STUDY_MODE_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class StudyModeTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<StudyModeTabHelper> {
 public:
  ~StudyModeTabHelper() override = default;

  // content::WebContentsObserver:
  void ResourceLoadComplete(
      content::RenderFrameHost* render_frame_host,
      const content::GlobalRequestID& request_id,
      const blink::mojom::ResourceLoadInfo& resource_load_info) override;

 private:
  friend class content::WebContentsUserData<StudyModeTabHelper>;
  explicit StudyModeTabHelper(content::WebContents* web_contents);
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // CHROME_BROWSER_STUDY_MODE_STUDY_MODE_TAB_HELPER_H_
