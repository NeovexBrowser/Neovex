// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://webui-test/chromeos/mojo_webui_test_support.js';
import 'neovex://scanning/multi_page_checkbox.js';

import {CrCheckboxElement} from 'neovex://resources/ash/common/cr_elements/cr_checkbox/cr_checkbox.js';
import {strictQuery} from 'neovex://resources/ash/common/typescript_utils/strict_query.js';
import {assert} from 'neovex://resources/js/assert.js';
import type {MultiPageCheckboxElement} from 'neovex://scanning/multi_page_checkbox.js';
import {assertFalse, assertTrue} from 'neovex://webui-test/chromeos/chai_assert.js';

suite('multiPageCheckboxTest', function() {
  let multiPageCheckbox: MultiPageCheckboxElement|null = null;

  setup(() => {
    multiPageCheckbox = document.createElement('multi-page-checkbox');
    assertTrue(!!multiPageCheckbox);
    document.body.appendChild(multiPageCheckbox);
  });

  teardown(() => {
    multiPageCheckbox?.remove();
    multiPageCheckbox = null;
  });

  // Verify that clicking the checkbox directly and clicking the text label can
  // both toggle the boolean.
  test('checkboxClicked', () => {
    assert(multiPageCheckbox);
    assertFalse(multiPageCheckbox.multiPageScanChecked);
    const checkbox = strictQuery(
        'cr-checkbox', multiPageCheckbox.shadowRoot, CrCheckboxElement);
    checkbox.click();
    assertTrue(multiPageCheckbox.multiPageScanChecked);
    checkbox.click();
    assertFalse(multiPageCheckbox.multiPageScanChecked);
  });
});
