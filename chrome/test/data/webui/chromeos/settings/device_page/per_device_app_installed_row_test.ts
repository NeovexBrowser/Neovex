// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/lazy_load.js';

import {PerDeviceAppInstalledRowElement} from 'neovex://os-settings/lazy_load.js';
import type {CompanionAppInfo} from 'neovex://os-settings/os_settings.js';
import {CompanionAppState, CrLinkRowElement} from 'neovex://os-settings/os_settings.js';
import {strictQuery} from 'neovex://resources/ash/common/typescript_utils/strict_query.js';
import {assertEquals} from 'neovex://webui-test/chai_assert.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';

import {clearBody} from '../utils.js';

const defaultAppInfo: CompanionAppInfo = {
  packageId: 'packageId',
  appName: 'AppName',
  actionLink: 'www.example123.com',
  iconUrl: 'data:image/png;base64,gg==',
  state: CompanionAppState.kInstalled,
};

suite(PerDeviceAppInstalledRowElement.is, () => {
  let appRow: PerDeviceAppInstalledRowElement;

  async function createAppRow(appInfo: CompanionAppInfo) {
    clearBody();
    appRow = document.createElement(PerDeviceAppInstalledRowElement.is);
    appRow.appInfo = appInfo;
    document.body.appendChild(appRow);
    return flushTasks();
  }

  test('Initialize per-device-app-installed-row', async () => {
    await createAppRow(defaultAppInfo);
    const openAppRow =
        strictQuery('#openApp', appRow.shadowRoot, CrLinkRowElement);
    assertEquals(`Open ${defaultAppInfo.appName}`, openAppRow.label);
  });
});
