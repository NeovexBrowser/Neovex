// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://diagnostics/overview_card.js';
import 'neovex://webui-test/chromeos/mojo_webui_test_support.js';

import {fakeSystemInfo, fakeSystemInfoWithoutBoardName, fakeSystemInfoWithTBD} from 'neovex://diagnostics/fake_data.js';
import {FakeSystemDataProvider} from 'neovex://diagnostics/fake_system_data_provider.js';
import {setSystemDataProviderForTesting} from 'neovex://diagnostics/mojo_interface_provider.js';
import {OverviewCardElement} from 'neovex://diagnostics/overview_card.js';
import type {SystemInfo} from 'neovex://diagnostics/system_data_provider.mojom-webui.js';
import {loadTimeData} from 'neovex://resources/ash/common/load_time_data.m.js';
import {strictQuery} from 'neovex://resources/ash/common/typescript_utils/strict_query.js';
import {assert} from 'neovex://resources/js/assert.js';
import {assertEquals, assertFalse, assertNotEquals} from 'neovex://webui-test/chromeos/chai_assert.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';
import {isVisible} from 'neovex://webui-test/test_util.js';

import * as dx_utils from './diagnostics_test_utils.js';

suite('overviewCardTestSuite', function() {
  let overviewElement: OverviewCardElement|null = null;

  const provider = new FakeSystemDataProvider();

  suiteSetup(() => {
    setSystemDataProviderForTesting(provider);
  });

  setup(() => {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
  });

  teardown(() => {
    overviewElement?.remove();
    overviewElement = null;
    provider.reset();
  });

  function initializeOverviewCard(fakeSystemInfo: SystemInfo): Promise<void> {
    assertFalse(!!overviewElement);

    // Initialize the fake data.
    provider.setFakeSystemInfo(fakeSystemInfo);

    // Add the overview card to the DOM.
    overviewElement = document.createElement(OverviewCardElement.is);
    assert(overviewElement);
    document.body.appendChild(overviewElement);

    return flushTasks();
  }

  test('OverviewCardPopulated', async () => {
    await initializeOverviewCard(fakeSystemInfo);
    assert(overviewElement);
    dx_utils.assertElementContainsText(
        strictQuery(
            '#marketingName', overviewElement.shadowRoot, HTMLSpanElement),
        fakeSystemInfo.marketingName);
    dx_utils.assertElementContainsText(
        strictQuery('#deviceInfo', overviewElement.shadowRoot, HTMLSpanElement),
        fakeSystemInfo.boardName);
    dx_utils.assertElementContainsText(
        strictQuery('#deviceInfo', overviewElement.shadowRoot, HTMLSpanElement),
        fakeSystemInfo.versionInfo.milestoneVersion);
  });

  test('TBDMarketingNameHidden', async () => {
    await initializeOverviewCard(fakeSystemInfoWithTBD);
    assert(overviewElement);
    assertFalse(isVisible(strictQuery(
        '#marketingName', overviewElement.shadowRoot, HTMLSpanElement)));
    // Device info should not be surrounded by parentheses when the marketing
    // name is hidden.
    const deviceInfoText =
        strictQuery('#deviceInfo', overviewElement.shadowRoot, HTMLSpanElement)
            .textContent;
    assert(deviceInfoText);
    assertNotEquals('(', deviceInfoText[0]);
    assertNotEquals(')', deviceInfoText[deviceInfoText.length - 1]);
  });

  test('BoardNameMissing', async () => {
    await initializeOverviewCard(fakeSystemInfoWithoutBoardName);
    assert(overviewElement);
    const versionInfo = loadTimeData.getStringF(
        'versionInfo',
        fakeSystemInfoWithoutBoardName.versionInfo.fullVersionString);
    assertEquals(
        versionInfo[0]!.toUpperCase() + versionInfo.slice(1),
        strictQuery('#deviceInfo', overviewElement.shadowRoot, HTMLSpanElement)
            .textContent);
  });
});
