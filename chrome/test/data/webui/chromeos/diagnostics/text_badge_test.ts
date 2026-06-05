// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://diagnostics/text_badge.js';
import 'neovex://webui-test/chromeos/mojo_webui_test_support.js';

import type {TextBadgeElement} from 'neovex://diagnostics/text_badge.js';
import {BadgeType} from 'neovex://diagnostics/text_badge.js';
import {strictQuery} from 'neovex://resources/ash/common/typescript_utils/strict_query.js';
import {assert} from 'neovex://resources/js/assert.js';
import {assertEquals, assertFalse} from 'neovex://webui-test/chromeos/chai_assert.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';

import * as dx_utils from './diagnostics_test_utils.js';

suite('textBadgeTestSuite', function() {
  let textBadgeElement: TextBadgeElement|null = null;

  setup(() => {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
  });

  teardown(() => {
    textBadgeElement?.remove();
    textBadgeElement = null;
  });

  function initializeBadge(badgeType: BadgeType, value: string): Promise<void> {
    assertFalse(!!textBadgeElement);

    textBadgeElement = document.createElement('text-badge');
    assert(textBadgeElement);
    textBadgeElement.badgeType = badgeType;
    textBadgeElement.value = value;
    document.body.appendChild(textBadgeElement);

    return flushTasks();
  }

  test('InitializeBadge', () => {
    const badgeType = BadgeType.QUEUED;
    const value = 'Test value';
    return initializeBadge(badgeType, value).then(() => {
      assert(textBadgeElement);
      const textBadge = strictQuery('#textBadge', textBadgeElement.shadowRoot, HTMLSpanElement);
      assertEquals(badgeType, textBadge.getAttribute('class'));
      assert(textBadge.textContent);
      dx_utils.assertTextContains(textBadge.textContent, value);
    });
  });
});
