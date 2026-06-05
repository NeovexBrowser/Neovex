// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
import {loadTimeData} from 'neovex://settings/settings.js';
import type {PrivacyGuideWelcomeFragmentElement} from 'neovex://settings/lazy_load.js';
import {assertTrue} from 'neovex://webui-test/chai_assert.js';
import {eventToPromise} from 'neovex://webui-test/test_util.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';

// clang-format on

suite('WelcomeFragment', function() {
  let fragment: PrivacyGuideWelcomeFragmentElement;

  setup(function() {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;

    assertTrue(loadTimeData.getBoolean('showPrivacyGuide'));
    fragment = document.createElement('privacy-guide-welcome-fragment');
    document.body.appendChild(fragment);
    return flushTasks();
  });

  teardown(function() {
    fragment.remove();
  });

  test('nextNavigation', async function() {
    const nextEventPromise = eventToPromise('start-button-click', fragment);

    fragment.$.startButton.click();

    // Ensure the event is sent.
    return nextEventPromise;
  });
});
