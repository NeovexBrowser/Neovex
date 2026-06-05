// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import type {HatsBrowserProxy, SafeBrowsingSetting, SecurityPageV2Interaction, SecuritySettingsBundleSetting, TrustSafetyInteraction} from 'neovex://settings/settings.js';
import {TestBrowserProxy} from 'neovex://webui-test/test_browser_proxy.js';

export class TestHatsBrowserProxy extends TestBrowserProxy implements
    HatsBrowserProxy {
  private currentTime: number;

  constructor() {
    super([
      'trustSafetyInteractionOccurred',
      'securityPageHatsRequest',
      'now',
    ]);
    this.currentTime = 0;
  }

  trustSafetyInteractionOccurred(interaction: TrustSafetyInteraction) {
    this.methodCalled('trustSafetyInteractionOccurred', interaction);
  }

  securityPageHatsRequest(
      securityPageInteractions: SecurityPageV2Interaction[],
      safeBrowsingSetting: SafeBrowsingSetting, totalTimeOnPage: number,
      securitySettingsBundleSetting: SecuritySettingsBundleSetting) {
    this.methodCalled('securityPageHatsRequest', [
      securityPageInteractions,
      safeBrowsingSetting,
      totalTimeOnPage,
      securitySettingsBundleSetting,
    ]);
  }

  setNow(now: number) {
    this.currentTime = now;
  }

  now() {
    this.methodCalled('now', this.currentTime);
    return this.currentTime;
  }
}
