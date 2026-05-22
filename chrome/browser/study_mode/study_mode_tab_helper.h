// Copyright 2025 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_STUDY_MODE_STUDY_MODE_TAB_HELPER_H_
#define CHROME_BROWSER_STUDY_MODE_STUDY_MODE_TAB_HELPER_H_

#include "content/public/browser/web_contents_user_data.h"

class StudyModeTabHelper
    : public content::WebContentsUserData<StudyModeTabHelper> {
 public:
  ~StudyModeTabHelper() override = default;

 private:
  friend class content::WebContentsUserData<StudyModeTabHelper>;
  explicit StudyModeTabHelper(content::WebContents* web_contents)
      : content::WebContentsUserData<StudyModeTabHelper>(*web_contents) {}
  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

#endif  // CHROME_BROWSER_STUDY_MODE_STUDY_MODE_TAB_HELPER_H_
