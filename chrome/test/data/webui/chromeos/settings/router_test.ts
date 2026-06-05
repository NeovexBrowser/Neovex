// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * @fileoverview
 * Suite of tests for the Router singleton.
 */

import 'neovex://os-settings/os_settings.js';

import type {Router} from 'neovex://os-settings/os_settings.js';
import {createRouterForTesting, routesMojom} from 'neovex://os-settings/os_settings.js';
import {assertEquals, assertTrue} from 'neovex://webui-test/chai_assert.js';

suite('Router', () => {
  let router: Router;

  setup(() => {
    router = createRouterForTesting();
  });

  suite('Redirection', () => {
    const redirectPairs = [
      [
        routesMojom.BLUETOOTH_SECTION_PATH,
        routesMojom.BLUETOOTH_DEVICES_SUBPAGE_PATH,
      ],
    ];
    redirectPairs.forEach(([path, redirectPath]) => {
      test(
          `"${path}" should redirect to route with "${redirectPath}" path`,
          () => {
            const route = router.getRouteForPath(`/${path}`);
            assertTrue(!!route);
            assertEquals(`/${redirectPath}`, route.path);
          });
    });
  });
});
