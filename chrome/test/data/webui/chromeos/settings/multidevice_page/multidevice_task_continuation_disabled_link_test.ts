// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/lazy_load.js';

import type {SettingsMultideviceTaskContinuationDisabledLinkElement} from 'neovex://os-settings/lazy_load.js';
import {Router, routes} from 'neovex://os-settings/os_settings.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertNotEquals, assertTrue} from 'neovex://webui-test/chai_assert.js';
import {eventToPromise} from 'neovex://webui-test/test_util.js';

suite('<settings-multidevice-task-continuation-disabled-link>', () => {
  let localizedLink: SettingsMultideviceTaskContinuationDisabledLinkElement;

  setup(() => {
    localizedLink = document.createElement(
        'settings-multidevice-task-continuation-disabled-link');
    document.body.appendChild(localizedLink);
    flush();
  });

  teardown(() => {
    localizedLink.remove();
    Router.getInstance().resetRouteForTesting();
  });

  test('Contains 2 links with aria-labels', () => {
    const chromeSyncLink =
        localizedLink.shadowRoot!.querySelector('#chromeSyncLink');
    assertTrue(!!chromeSyncLink);
    assertTrue(chromeSyncLink.hasAttribute('aria-label'));
    const learnMoreLink =
        localizedLink.shadowRoot!.querySelector('#learnMoreLink');
    assertTrue(!!learnMoreLink);
    assertTrue(learnMoreLink.hasAttribute('aria-label'));
  });

  test('Spans are aria-hidden', () => {
    const spans = localizedLink.shadowRoot!.querySelectorAll('span');
    spans.forEach((span) => {
      assertTrue(span.hasAttribute('aria-hidden'));
    });
  });

  test('ChromeSyncLink navigates to appropriate route', async () => {
    const chromeSyncLink =
        localizedLink.shadowRoot!.querySelector<HTMLAnchorElement>(
            '#chromeSyncLink');
    assertTrue(!!chromeSyncLink);
    const advancedSyncOpenedPromise =
        eventToPromise('opened-browser-advanced-sync-settings', localizedLink);

    chromeSyncLink.click();

    await advancedSyncOpenedPromise;
    assertNotEquals(Router.getInstance().currentRoute, routes.OS_SYNC_CONTROLS);
  });
});
