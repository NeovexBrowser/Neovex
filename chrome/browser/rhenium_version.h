// Copyright 2026 The Rhenium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_RHENIUM_VERSION_H_
#define CHROME_BROWSER_RHENIUM_VERSION_H_

// Semantic version of the Rhenium Browser.
// Bumping this version will:
// 1. Display the new version on chrome://settings/help
// 2. Show chrome://rhenium-whatsnew/ once on the next browser launch
// 3. Report this version to the auto-updater
#define RHENIUM_VERSION "1.3"
#define RHENIUM_RELEASE_TAG "v1.3"
#define RHENIUM_GITHUB_RELEASES_API \
  "https://api.github.com/repos/RheniumBrowser/Rhenium/releases/latest"

#endif  // CHROME_BROWSER_RHENIUM_VERSION_H_
