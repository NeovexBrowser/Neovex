// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/lazy_load.js';

import {PerDeviceInstallRowElement} from 'neovex://os-settings/lazy_load.js';
import type {CompanionAppInfo, CrAutoImgElement} from 'neovex://os-settings/os_settings.js';
import {CompanionAppState} from 'neovex://os-settings/os_settings.js';
import {strictQuery} from 'neovex://resources/ash/common/typescript_utils/strict_query.js';
import {assertEquals} from 'neovex://webui-test/chai_assert.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';

import {clearBody} from '../utils.js';

const defaultAppInfo: CompanionAppInfo = {
  packageId: 'packageId',
  appName: 'AppName',
  actionLink: 'www.example123.com',
  iconUrl: 'data:image/png;base64,gg==',
  state: CompanionAppState.kAvailable,
};

suite(PerDeviceInstallRowElement.is, () => {
  let installRow: PerDeviceInstallRowElement;

  async function createInstallRow(appInfo: CompanionAppInfo) {
    clearBody();
    installRow = document.createElement(PerDeviceInstallRowElement.is);
    installRow.appInfo = appInfo;
    document.body.appendChild(installRow);
    return flushTasks();
  }

  test('App label and button displayed correctly', async () => {
    await createInstallRow(defaultAppInfo);
    const appLabel =
        strictQuery('#appName', installRow.shadowRoot, HTMLSpanElement);
    assertEquals(
        `Install ${defaultAppInfo.appName}`, appLabel.textContent.trim());
  });

  test('App image is loaded', async () => {
    await createInstallRow(defaultAppInfo);
    assertEquals(
        defaultAppInfo.iconUrl,
        installRow.shadowRoot!.querySelector<CrAutoImgElement>('img')!.autoSrc);
  });
});
