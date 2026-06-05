// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/lazy_load.js';

import type {SettingsPairedBluetoothListElement} from 'neovex://os-settings/lazy_load.js';
import type {PaperTooltipElement} from 'neovex://os-settings/os_settings.js';
import {DeviceConnectionState} from 'neovex://resources/mojo/chromeos/ash/services/bluetooth_config/public/mojom/cros_bluetooth_config.mojom-webui.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertEquals, assertFalse, assertTrue} from 'neovex://webui-test/chai_assert.js';
import {createDefaultBluetoothDevice} from 'neovex://webui-test/chromeos/bluetooth/fake_bluetooth_config.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';
import {eventToPromise} from 'neovex://webui-test/test_util.js';

suite('<os-settings-paired-bluetooth-list>', () => {
  let pairedBluetoothList: SettingsPairedBluetoothListElement;

  setup(() => {
    pairedBluetoothList =
        document.createElement('os-settings-paired-bluetooth-list');
    document.body.appendChild(pairedBluetoothList);
    flush();
  });

  teardown(() => {
    pairedBluetoothList.remove();
  });

  test('Base Test', () => {
    const list = pairedBluetoothList.shadowRoot!.querySelector('iron-list');
    assertTrue(!!list);
  });

  test('Device list change renders items correctly', async () => {
    const device = createDefaultBluetoothDevice(
        /*id=*/ '123456789', /*publicName=*/ 'BeatsX',
        /*connectionState=*/
        DeviceConnectionState.kConnected);

    pairedBluetoothList.set('devices', [device, device, device]);
    await flushTasks();

    const getListItems = () => {
      return pairedBluetoothList.shadowRoot!.querySelectorAll(
          'os-settings-paired-bluetooth-list-item');
    };
    assertEquals(3, getListItems().length);

    const ironResizePromise =
        eventToPromise('iron-resize', pairedBluetoothList);
    pairedBluetoothList.set(
        'devices', [device, device, device, device, device]);

    await ironResizePromise;
    flush();
    assertEquals(5, getListItems().length);
  });

  test('Tooltip is shown', async () => {
    const getTooltip = () => {
      const tooltip =
          pairedBluetoothList.shadowRoot!.querySelector<PaperTooltipElement>(
              '#tooltip');
      assertTrue(!!tooltip);
      return tooltip;
    };

    assertFalse(getTooltip().get('_showing'));
    const device = createDefaultBluetoothDevice(
        /*id=*/ '123456789', /*publicName=*/ 'BeatsX',
        /*connectionState=*/
        DeviceConnectionState.kConnected);

    pairedBluetoothList.set('devices', [device]);
    await flushTasks();

    const listItem = pairedBluetoothList.shadowRoot!.querySelector(
        'os-settings-paired-bluetooth-list-item');
    assertTrue(!!listItem);

    listItem.dispatchEvent(new CustomEvent('managed-tooltip-state-change', {
      bubbles: true,
      composed: true,
      detail: {
        address: 'device-address',
        show: true,
        element: document.createElement('div'),
      },
    }));

    await flushTasks();
    assertTrue(getTooltip().get('_showing'));

    listItem.dispatchEvent(new CustomEvent('managed-tooltip-state-change', {
      bubbles: true,
      composed: true,
      detail: {
        address: 'device-address',
        show: false,
        element: document.createElement('div'),
      },
    }));
    await flushTasks();
    assertFalse(getTooltip().get('_showing'));
  });
});
