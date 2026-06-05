// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/lazy_load.js';

import type {SettingsPrivacyHubGeolocationAdvancedSubpage} from 'neovex://os-settings/lazy_load.js';
import type {SettingsToggleButtonElement} from 'neovex://os-settings/os_settings.js';
import {Router, routes} from 'neovex://os-settings/os_settings.js';
import {assert} from 'neovex://resources/js/assert.js';
import {loadTimeData} from 'neovex://resources/js/load_time_data.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertEquals, assertFalse, assertNull, assertTrue} from 'neovex://webui-test/chai_assert.js';

suite('<settings-privacy-hub-geolocation-advanced-subpage>', () => {
  let page: SettingsPrivacyHubGeolocationAdvancedSubpage;

  // Helper functions
  function initPage(showPrivacyHubLocationControl: boolean) {
    loadTimeData.overrideValues({
      showPrivacyHubLocationControl: showPrivacyHubLocationControl,
    });

    page = document.createElement(
        'settings-privacy-hub-geolocation-advanced-subpage');
    const prefs = {
      'ash': {
        'user': {
          'geolocation_accuracy_enabled': {
            value: true,
          },
        },
      },
    };
    page.prefs = prefs;
    document.body.appendChild(page);
    return Promise.resolve();
  }

  setup(() => {
    Router.getInstance().navigateTo(routes.PRIVACY_HUB_GEOLOCATION_ADVANCED);
  });

  teardown(() => {
    page.remove();
    Router.getInstance().resetRouteForTesting();
  });

  // Function to get the toggle.
  function getGeoLocationAccuracyToggle(): SettingsToggleButtonElement|null {
    return page.shadowRoot!.querySelector<SettingsToggleButtonElement>(
        '#geoLocationAccuracyToggle');
  }

  // Tests
  test('Validate label matches on the page', async () => {
    await initPage(/** showPrivacyHubLocationControl */ true);
    const toggle = getGeoLocationAccuracyToggle();
    assert(toggle);
    assertEquals(page.i18n('geolocationAccuracyToggleTitle'), toggle.label);
  });

  test('Validate toggle updates the pref', async () => {
    await initPage(/** showPrivacyHubLocationControl */ true);
    const toggle = getGeoLocationAccuracyToggle();
    assert(toggle);
    assertTrue(page.prefs.ash.user.geolocation_accuracy_enabled.value);
    toggle.click();
    flush();
    assertFalse(page.prefs.ash.user.geolocation_accuracy_enabled.value);
  });

  test('Validate toggle not shown when setting is false', async () => {
    await initPage(/** showPrivacyHubLocationControl */ false);
    const toggle = getGeoLocationAccuracyToggle();
    assertNull(toggle);
  });

  test('Location accuracy is disabled for secondary users', async () => {
    // Simulate secondary user flow.
    loadTimeData.overrideValues({
      isSecondaryUser: true,
    });
    await initPage(/** showPrivacyHubLocationControl */ true);

    assertTrue(getGeoLocationAccuracyToggle()!.disabled);
  });
});
