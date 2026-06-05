// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/lazy_load.js';

import type {SettingsLockScreenElement} from 'neovex://os-settings/lazy_load.js';
import {Router, routes} from 'neovex://os-settings/os_settings.js';
import {getDeepActiveElement} from 'neovex://resources/js/util.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertEquals, assertTrue} from 'neovex://webui-test/chai_assert.js';
import {waitAfterNextRender} from 'neovex://webui-test/polymer_test_util.js';

suite('<settings-lock-screen-subpage>', function() {
  let lockScreenPage: SettingsLockScreenElement|null = null;

  setup(function() {
    lockScreenPage = document.createElement('settings-lock-screen-subpage');
    document.body.appendChild(lockScreenPage);
    flush();
  });

  teardown(function() {
    lockScreenPage!.remove();
    Router.getInstance().resetRouteForTesting();
  });

  test('Deep link to Lock screen', async () => {
    const settingId = '1109';

    const params = new URLSearchParams();
    params.append('settingId', settingId);
    Router.getInstance().navigateTo(routes.LOCK_SCREEN, params);

    flush();

    const deepLinkElement =
        lockScreenPage!.shadowRoot!.querySelector('#enableLockScreen')!
            .shadowRoot!.querySelector('cr-toggle');
    assertTrue(!!deepLinkElement);
    await waitAfterNextRender(deepLinkElement);
    assertEquals(
        deepLinkElement, getDeepActiveElement(),
        'Lock screen toggle should be focused for settingId=' + settingId);
  });
});
