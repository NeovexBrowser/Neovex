// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/strings.m.js';
import 'neovex://resources/ash/common/cellular_setup/cellular_setup.js';
import 'neovex://resources/ash/common/cellular_setup/psim_flow_ui.js';

import type {CellularSetupElement} from 'neovex://resources/ash/common/cellular_setup/cellular_setup.js';
import {CellularSetupPageName} from 'neovex://resources/ash/common/cellular_setup/cellular_types.js';
import {setESimManagerRemoteForTesting} from 'neovex://resources/ash/common/cellular_setup/mojo_interface_provider.js';
import {MojoInterfaceProviderImpl} from 'neovex://resources/ash/common/network/mojo_interface_provider.js';
import {InhibitReason} from 'neovex://resources/mojo/chromeos/services/network_config/public/mojom/cros_network_config.mojom-webui.js';
import {DeviceStateType, NetworkType} from 'neovex://resources/mojo/chromeos/services/network_config/public/mojom/network_types.mojom-webui.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertFalse, assertTrue} from 'neovex://webui-test/chai_assert.js';

import {FakeNetworkConfig} from '../fake_network_config_mojom.js';

import {FakeCellularSetupDelegate} from './fake_cellular_setup_delegate.js';
import {FakeESimManagerRemote} from './fake_esim_manager_remote.js';

suite('CrComponentsCellularSetupTest', function() {
  let cellularSetupPage: CellularSetupElement;
  let eSimManagerRemote: FakeESimManagerRemote;
  let networkConfigRemote: FakeNetworkConfig;

  setup(function() {
    eSimManagerRemote = new FakeESimManagerRemote();
    setESimManagerRemoteForTesting(eSimManagerRemote);

    networkConfigRemote = new FakeNetworkConfig();
    MojoInterfaceProviderImpl.getInstance().setMojoServiceRemoteForTest(
        networkConfigRemote);
  });

  async function flushAsync() {
    flush();
    // Use setTimeout to wait for the next macrotask.
    return new Promise(resolve => setTimeout(resolve));
  }

  function init() {
    networkConfigRemote.setDeviceStateForTest({
      ipv4Address: null,
      ipv6Address: null,
      imei: null,
      macAddress: null,
      scanning: false,
      simLockStatus: null,
      inhibitReason: InhibitReason.kNotInhibited,
      simAbsent: false,
      managedNetworkAvailable: false,
      serial: null,
      isCarrierLocked: false,
      isFlashing: false,
      type: NetworkType.kCellular,
      deviceState: DeviceStateType.kEnabled,
      simInfos: [{
        slotId: 0,
        iccid: '1111111111111111',
        eid: '',
        isPrimary: false,
      }],
    });
    eSimManagerRemote.addEuiccForTest(2);
    flush();

    cellularSetupPage = document.createElement('cellular-setup');
    cellularSetupPage.delegate = new FakeCellularSetupDelegate();
    document.body.appendChild(cellularSetupPage);
    flush();
  }

  test('Show pSim flow ui', async function() {
    init();
    await flushAsync();
    let eSimFlow = cellularSetupPage.shadowRoot!.querySelector('esim-flow-ui');
    let pSimFlow = cellularSetupPage.shadowRoot!.querySelector('psim-flow-ui');

    assertTrue(!!eSimFlow);
    assertFalse(!!pSimFlow);

    cellularSetupPage.currentPageName = CellularSetupPageName.PSIM_FLOW_UI;
    await flushAsync();
    eSimFlow = cellularSetupPage.shadowRoot!.querySelector('esim-flow-ui');
    pSimFlow = cellularSetupPage.shadowRoot!.querySelector('psim-flow-ui');

    assertFalse(!!eSimFlow);
    assertTrue(!!pSimFlow);
  });

  test('Show eSIM flow ui', async function() {
    init();
    await flushAsync();
    const eSimFlow =
        cellularSetupPage.shadowRoot!.querySelector('esim-flow-ui');
    const pSimFlow =
        cellularSetupPage.shadowRoot!.querySelector('psim-flow-ui');

    // By default eSIM flow is always shown
    assertTrue(!!eSimFlow);
    assertFalse(!!pSimFlow);
  });
});
