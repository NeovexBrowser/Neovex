// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/color/material_tab_strip_color_mixer.h"

#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/color/chrome_color_provider_utils.h"
#include "ui/color/color_id.h"
#include "ui/color/color_mixer.h"
#include "ui/color/color_provider.h"
#include "ui/color/color_recipe.h"


void AddMaterialTabStripColorMixer(ui::ColorProvider* provider,
                                   const ui::ColorProviderKey& key) {
  if (!ShouldApplyChromeMaterialOverrides(key)) {
    return;
  }

  // === CUSTOM THEME: NTP-matched dark palette ===
  // Background: #0b0b14  Toolbar: #12121e  Active tab: #1a1a2e
  // Accents: purple #5838A0, blue #1E4082, teal #286E78
  constexpr SkColor kFrameBg = SkColorSetRGB(0x0b, 0x0b, 0x14);
  constexpr SkColor kActiveTabBg = SkColorSetRGB(0x1a, 0x1a, 0x2e);
  constexpr SkColor kActiveTabHover = SkColorSetRGB(0x25, 0x25, 0x40);
  constexpr SkColor kInactiveTabBg = kFrameBg;  // transparent against frame
  constexpr SkColor kInactiveTabHover = SkColorSetRGB(0x1a, 0x1a, 0x2e);
  constexpr SkColor kSelectedTabBg = SkColorSetRGB(0x16, 0x16, 0x2a);
  constexpr SkColor kTabTextActive = SkColorSetRGB(0xe0, 0xe0, 0xff);
  constexpr SkColor kTabTextInactive = SkColorSetRGB(0x88, 0x8a, 0xaa);
  constexpr SkColor kDivider = SkColorSetRGB(0x2a, 0x2a, 0x44);
  constexpr SkColor kControlBtnBg = SkColorSetRGB(0x16, 0x16, 0x2a);
  constexpr SkColor kControlBtnIcon = SkColorSetRGB(0x95, 0x95, 0xb8);

  ui::ColorMixer& mixer = provider->AddMixer();

  // Active tab backgrounds.
  mixer[kColorDetachedTabBackgroundActiveFrameActive] = {kActiveTabBg};
  mixer[kColorTabBackgroundActiveFrameActive] = {kActiveTabBg};
  mixer[kColorTabBackgroundActiveFrameInactive] = {kActiveTabBg};

  // Inactive tab backgrounds.
  mixer[kColorTabBackgroundInactiveFrameActive] = {kInactiveTabBg};
  mixer[kColorTabBackgroundInactiveFrameInactive] = {kInactiveTabBg};

  // Hover states.
  mixer[kColorTabBackgroundInactiveHoverFrameActive] = {kInactiveTabHover};
  mixer[kColorTabBackgroundInactiveHoverFrameInactive] = {kInactiveTabHover};

  // Selected tab states.
  mixer[kColorTabBackgroundSelectedFrameActive] = {kSelectedTabBg};
  mixer[kColorTabBackgroundSelectedFrameInactive] = {kSelectedTabBg};
  mixer[kColorTabBackgroundSelectedHoverFrameActive] = {kActiveTabHover};
  mixer[kColorTabBackgroundSelectedHoverFrameInactive] = {kActiveTabHover};

  // Tab text colors.
  mixer[kColorTabForegroundActiveFrameActive] = {kTabTextActive};
  mixer[kColorTabForegroundActiveFrameInactive] = {kTabTextActive};
  mixer[kColorTabForegroundInactiveFrameActive] = {kTabTextInactive};
  mixer[kColorTabForegroundInactiveFrameInactive] = {kTabTextInactive};

  // Tab dividers.
  mixer[kColorTabDividerFrameActive] = {kDivider};
  mixer[kColorTabDividerFrameInactive] = {kDivider};

  // Tab strip combo / control buttons.
  mixer[kColorTabStripComboButtonSeparator] = {kDivider};
  mixer[kColorTabStripControlButtonInkDrop] = {kInactiveTabHover};
  mixer[kColorTabStripControlButtonInkDropRipple] = {
      SkColorSetA(kControlBtnIcon, 0x30)};

#if !BUILDFLAG(IS_ANDROID)
  mixer[kColorTabDiscardRingFrameActive] = {kDivider};
  mixer[kColorTabDiscardRingFrameInactive] = {kDivider};
#endif

  /* WebUI Tab Strip colors. */
  mixer[kColorWebUiTabStripBackground] = {kFrameBg};
  mixer[kColorWebUiTabStripFocusOutline] = {SkColorSetRGB(0x58, 0x38, 0xA0)};
  mixer[kColorWebUiTabStripScrollbarThumb] = {
      SkColorSetA(kControlBtnIcon, 0xB3)};
  mixer[kColorWebUiTabStripTabActiveTitleBackground] = {
      SkColorSetRGB(0x58, 0x38, 0xA0)};
  mixer[kColorWebUiTabStripTabActiveTitleContent] = {kTabTextActive};
  mixer[kColorWebUiTabStripTabBackground] = {kActiveTabBg};
  mixer[kColorWebUiTabStripTabSeparator] = {SkColorSetA(kDivider, 0x60)};
  mixer[kColorWebUiTabStripTabText] = {kTabTextActive};

  // New Tab / Tab Search button colors.
  mixer[kColorNewTabButtonCRForegroundFrameActive] = {kControlBtnIcon};
  mixer[kColorNewTabButtonCRForegroundFrameInactive] = {kControlBtnIcon};
  mixer[kColorNewTabButtonCRBackgroundFrameActive] = {kControlBtnBg};
  mixer[kColorNewTabButtonCRBackgroundFrameInactive] = {kControlBtnBg};

  mixer[kColorTabSearchButtonCRForegroundFrameActive] = {kControlBtnIcon};
  mixer[kColorTabSearchButtonCRForegroundFrameInactive] = {kControlBtnIcon};
}

