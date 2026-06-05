// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/os_settings.js';

import type {SettingsBluetoothPairingDialogElement} from 'neovex://os-settings/os_settings.js';
import {setBluetoothConfigForTesting} from 'neovex://resources/ash/common/bluetooth/cros_bluetooth_config.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertTrue} from 'neovex://webui-test/chai_assert.js';
import {FakeBluetoothConfig} from 'neovex://webui-test/chromeos/bluetooth/fake_bluetooth_config.js';
import {eventToPromise} from 'neovex://webui-test/test_util.js';

suite('<os-settings-bluetooth-pairing-dialog>', () => {
  let bluetoothPairingDialog: SettingsBluetoothPairingDialogElement;
  let bluetoothConfig: FakeBluetoothConfig;

  setup(() => {
    bluetoothConfig = new FakeBluetoothConfig();
    setBluetoothConfigForTesting(bluetoothConfig);

    bluetoothPairingDialog =
        document.createElement('os-settings-bluetooth-pairing-dialog');
    document.body.appendChild(bluetoothPairingDialog);
    flush();
  });

  teardown(() => {
    bluetoothPairingDialog.remove();
  });

  test('Finished event is fired on close', async () => {
    const pairingDialog = bluetoothPairingDialog.shadowRoot!.querySelector(
        'bluetooth-pairing-ui');
    assertTrue(!!pairingDialog);

    const closeBluetoothPairingUiPromise =
        eventToPromise('close', bluetoothPairingDialog);

    pairingDialog.dispatchEvent(new CustomEvent('finished'));

    await closeBluetoothPairingUiPromise;
  });
});
