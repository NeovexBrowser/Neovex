// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/lazy_load.js';

import type {SettingsMultideviceTaskContinuationItemElement} from 'neovex://os-settings/lazy_load.js';
import {SyncBrowserProxyImpl} from 'neovex://os-settings/os_settings.js';
import {webUIListenerCallback} from 'neovex://resources/ash/common/cr.m.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertEquals, assertTrue} from 'neovex://webui-test/chai_assert.js';

import {TestSyncBrowserProxy} from '../test_os_sync_browser_proxy.js';

function getPrefs() {
  return {
    tabsSynced: true,
  };
}

suite('<settings-multidevice-task-continuation-item>', () => {
  let taskContinuationItem: SettingsMultideviceTaskContinuationItemElement;

  setup(() => {
    const browserProxy = new TestSyncBrowserProxy();
    SyncBrowserProxyImpl.setInstance(browserProxy);

    taskContinuationItem =
        document.createElement('settings-multidevice-task-continuation-item');
    document.body.appendChild(taskContinuationItem);

    flush();
  });

  teardown(() => {
    taskContinuationItem.remove();
  });

  test('Chrome Sync off', () => {
    const prefs = getPrefs();
    prefs.tabsSynced = false;
    flush();

    webUIListenerCallback('sync-prefs-changed', prefs);
    flush();

    assertTrue(!!taskContinuationItem.shadowRoot!.querySelector(
        'settings-multidevice-task-continuation-disabled-link'));

    const toggle = taskContinuationItem.shadowRoot!.querySelector('cr-toggle');
    assertTrue(!!toggle);
    assertTrue(toggle.disabled);
  });

  test('Chrome Sync on', () => {
    const prefs = getPrefs();
    prefs.tabsSynced = true;
    webUIListenerCallback('sync-prefs-changed', prefs);
    flush();

    assertEquals(
        null,
        taskContinuationItem.shadowRoot!.querySelector(
            'settings-multidevice-task-continuation-disabled-link'));
  });
});
