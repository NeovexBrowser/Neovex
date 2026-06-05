// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/lazy_load.js';

import type {SettingsConsumerAutoUpdateToggleDialogElement} from 'neovex://os-settings/lazy_load.js';
import {AboutPageBrowserProxyImpl} from 'neovex://os-settings/os_settings.js';
import {assertEquals, assertTrue} from 'neovex://webui-test/chai_assert.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';
import {eventToPromise} from 'neovex://webui-test/test_util.js';

import {clearBody} from '../utils.js';

import {TestAboutPageBrowserProxy} from './test_about_page_browser_proxy.js';

suite('<settings-consumer-auto-update-toggle-dialog>', () => {
  let dialog: SettingsConsumerAutoUpdateToggleDialogElement;
  let browserProxy: TestAboutPageBrowserProxy;

  setup(async () => {
    browserProxy = new TestAboutPageBrowserProxy();
    AboutPageBrowserProxyImpl.setInstanceForTesting(browserProxy);

    clearBody();
    dialog =
        document.createElement('settings-consumer-auto-update-toggle-dialog');
    document.body.appendChild(dialog);
    await flushTasks();
  });

  async function assertButtonClickFiresUpdateEvent(
      buttonId: string, shouldEnable: boolean): Promise<void> {
    const whenSetConsumerAutoUpdateFired =
        eventToPromise('set-consumer-auto-update', dialog);
    const button = dialog.shadowRoot!.querySelector<HTMLElement>(buttonId);
    assertTrue(!!button);
    button.click();
    const event = await whenSetConsumerAutoUpdateFired;
    assertEquals(shouldEnable, event.detail.item);
  }

  test('click turn off button fires disable event', async () => {
    await assertButtonClickFiresUpdateEvent(
        /*buttonId=*/ '#turnOffButton', /*shouldEnable=*/ false);
  });

  test('click keep updates button fires enable event', async () => {
    await assertButtonClickFiresUpdateEvent(
        /*buttonId=*/ '#keepUpdatesButton', /*shouldEnable=*/ true);
  });
});
