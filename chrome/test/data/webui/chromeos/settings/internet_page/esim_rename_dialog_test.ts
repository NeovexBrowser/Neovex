// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-settings/os_settings.js';

import type {CrInputElement, EsimRenameDialogElement} from 'neovex://os-settings/os_settings.js';
import {setESimManagerRemoteForTesting} from 'neovex://resources/ash/common/cellular_setup/mojo_interface_provider.js';
import {MojoInterfaceProviderImpl} from 'neovex://resources/ash/common/network/mojo_interface_provider.js';
import {OncMojo} from 'neovex://resources/ash/common/network/onc_mojo.js';
import {getDeepActiveElement} from 'neovex://resources/js/util.js';
import type {ESimManagerRemote} from 'neovex://resources/mojo/chromeos/ash/services/cellular_setup/public/mojom/esim_manager.mojom-webui.js';
import {ESimOperationResult} from 'neovex://resources/mojo/chromeos/ash/services/cellular_setup/public/mojom/esim_manager.mojom-webui.js';
import {NetworkType} from 'neovex://resources/mojo/chromeos/services/network_config/public/mojom/network_types.mojom-webui.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import {assertEquals, assertFalse, assertNotEquals, assertTrue} from 'neovex://webui-test/chai_assert.js';
import type {FakeProfile} from 'neovex://webui-test/chromeos/cellular_setup/fake_esim_manager_remote.js';
import {FakeESimManagerRemote} from 'neovex://webui-test/chromeos/cellular_setup/fake_esim_manager_remote.js';
import {FakeNetworkConfig} from 'neovex://webui-test/chromeos/fake_network_config_mojom.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';
import {eventToPromise, isVisible} from 'neovex://webui-test/test_util.js';

suite('<esim-rename-dialog>', () => {
  const TEST_CELLULAR_GUID = 'cellular_guid';

  let esimRenameDialog: EsimRenameDialogElement;
  let eSimManagerRemote: FakeESimManagerRemote;
  let mojoApi: FakeNetworkConfig;

  setup(() => {
    eSimManagerRemote = new FakeESimManagerRemote();
    setESimManagerRemoteForTesting(
        eSimManagerRemote as unknown as ESimManagerRemote);

    mojoApi = new FakeNetworkConfig();
    MojoInterfaceProviderImpl.getInstance().setMojoServiceRemoteForTest(
        mojoApi);
    mojoApi.resetForTest();
  });

  teardown(() => {
    esimRenameDialog.remove();
  });

  async function init(): Promise<void> {
    esimRenameDialog = document.createElement('esim-rename-dialog');
    const response = await mojoApi.getNetworkState(TEST_CELLULAR_GUID);
    esimRenameDialog.networkState = response.result;
    document.body.appendChild(esimRenameDialog);
    assertTrue(!!esimRenameDialog);
    await flushTasks();
    const eSimProfileName =
        esimRenameDialog.shadowRoot!.querySelector('#eSimprofileName');
    assertTrue(!!eSimProfileName);
    assertEquals(
        eSimProfileName.shadowRoot!.querySelector('input'),
        getDeepActiveElement());
  }

  function addEsimCellularNetwork(guid: string, iccid: string): void {
    const cellular = OncMojo.getDefaultManagedProperties(
        NetworkType.kCellular, guid, 'profile' + iccid);
    cellular.typeProperties.cellular!.iccid = iccid;
    cellular.typeProperties.cellular!.eid = iccid + 'eid';
    mojoApi.setManagedPropertiesForTest(cellular);
  }

  /**
   * @param value The value of the input
   * @param invalid If the input is invalid or not
   * @param inputCount The length of value in string
   *     format, with 2 digits
   */
  function assertInput(
      value: string, invalid: boolean, valueLength: string): void {
    const inputBox = esimRenameDialog.shadowRoot!.querySelector<CrInputElement>(
        '#eSimprofileName');
    const inputCount =
        esimRenameDialog.shadowRoot!.querySelector('#inputCount');
    assertTrue(!!inputBox);
    assertTrue(!!inputCount);

    assertEquals(value, inputBox.value);
    assertEquals(invalid, inputBox.invalid);
    const characterCountText = esimRenameDialog.i18n(
        'eSimRenameProfileInputCharacterCount', valueLength, 20);
    assertEquals(characterCountText, inputCount.textContent?.trim());
    assertEquals(
        esimRenameDialog.i18n('eSimRenameProfileInputA11yLabel', 20),
        inputBox.ariaDescription);
  }

  test('Rename esim profile', async () => {
    eSimManagerRemote.addEuiccForTest(1);
    addEsimCellularNetwork(TEST_CELLULAR_GUID, '1');
    await flushTasks();
    await init();

    const inputBox = esimRenameDialog.shadowRoot!.querySelector<CrInputElement>(
        '#eSimprofileName');
    assertTrue(!!inputBox);
    const profileName = inputBox.value;
    assertEquals('profile1', profileName);
    inputBox.value = 'new profile nickname';
    await flushTasks();
    const doneBtn =
        esimRenameDialog.shadowRoot!.querySelector<HTMLButtonElement>('#done');
    assertTrue(!!doneBtn);
    doneBtn.click();
    await flushTasks();
    const euicc = (await eSimManagerRemote.getAvailableEuiccs()).euiccs[0];
    assertTrue(!!euicc);
    const profile = (await euicc.getProfileList()).profiles[0];
    assertTrue(!!profile);
    const profileProperties = (await profile.getProperties()).properties;
    assertEquals('new profile nickname', profileProperties.nickname);
  });

  test('esimProfileRemote_ falsey, show error', async () => {
    eSimManagerRemote.addEuiccForTest(1);
    addEsimCellularNetwork(TEST_CELLULAR_GUID, '1');
    await flushTasks();

    esimRenameDialog = document.createElement('esim-rename-dialog');
    const response = await mojoApi.getNetworkState(TEST_CELLULAR_GUID);
    esimRenameDialog.networkState = response.result;
    // Setting iccid to null wil result in improper initialization.
    esimRenameDialog.set('networkState.typeState.cellular.iccid', null);
    document.body.appendChild(esimRenameDialog);
    assertTrue(!!esimRenameDialog);
    flush();

    await flushTasks();
    const doneBtn =
        esimRenameDialog.shadowRoot!.querySelector<HTMLButtonElement>('#done');
    assertTrue(!!doneBtn);
    assertFalse(doneBtn.disabled);
    const errorMessage =
        esimRenameDialog.shadowRoot!.querySelector('#errorMessage');
    assertTrue(!!errorMessage);
    assertTrue(isVisible(errorMessage));
  });

  test('Rename esim profile fails', async () => {
    eSimManagerRemote.addEuiccForTest(1);
    addEsimCellularNetwork(TEST_CELLULAR_GUID, '1');
    await flushTasks();
    await init();

    const inputBox = esimRenameDialog.shadowRoot!.querySelector<CrInputElement>(
        '#eSimprofileName');
    assertTrue(!!inputBox);
    const profileName = inputBox.value;
    assertEquals('profile1', profileName);
    const errorMessage =
        esimRenameDialog.shadowRoot!.querySelector('#errorMessage');
    assertTrue(!!errorMessage);
    assertFalse(isVisible(errorMessage));
    const euicc = (await eSimManagerRemote.getAvailableEuiccs()).euiccs[0];
    assertTrue(!!euicc);
    const profile = (await euicc.getProfileList()).profiles[0];
    assertTrue(!!profile);
    (profile as unknown as FakeProfile)
        .setEsimOperationResultForTest(ESimOperationResult.kFailure);
    inputBox.value = 'new profile nickname';
    await flushTasks();
    const showErrorToastPromise =
        eventToPromise('show-error-toast', esimRenameDialog);
    const doneBtn =
        esimRenameDialog.shadowRoot!.querySelector<HTMLButtonElement>('#done');
    const cancelBtn =
        esimRenameDialog.shadowRoot!.querySelector<HTMLButtonElement>(
            '#cancel');
    assertTrue(!!doneBtn);
    assertTrue(!!cancelBtn);
    assertFalse(doneBtn.disabled);
    assertFalse(cancelBtn.disabled);
    assertFalse(inputBox.disabled);
    doneBtn.click();
    await flushTasks();
    assertTrue(doneBtn.disabled);
    assertTrue(cancelBtn.disabled);
    assertTrue(inputBox.disabled);
    (profile as unknown as FakeProfile)['resolveSetProfileNicknamePromise']();
    await flushTasks();
    assertFalse(doneBtn.disabled);
    assertFalse(cancelBtn.disabled);
    assertFalse(inputBox.disabled);
    const profileProperties = (await profile.getProperties()).properties;
    const showErrorToastEvent = await showErrorToastPromise;
    assertEquals(
        esimRenameDialog.i18n('eSimRenameProfileDialogErrorToast'),
        showErrorToastEvent.detail);
    assertNotEquals('new profile nickname', profileProperties.nickname);
  });

  test('Warning message visibility', () => {
    const warningMessage =
        esimRenameDialog.shadowRoot!.querySelector<HTMLElement>(
            '#warningMessage');
    assertTrue(!!warningMessage);

    esimRenameDialog.showCellularDisconnectWarning = false;
    assertTrue(warningMessage.hidden);

    esimRenameDialog.showCellularDisconnectWarning = true;
    assertFalse(warningMessage.hidden);
  });

  test('Input is sanitized', async () => {
    eSimManagerRemote.addEuiccForTest(1);
    addEsimCellularNetwork(TEST_CELLULAR_GUID, '1');
    await flushTasks();
    await init();

    const inputBox = esimRenameDialog.shadowRoot!.querySelector<CrInputElement>(
        '#eSimprofileName');
    assertTrue(!!inputBox);
    const profileName = inputBox.value;
    assertEquals('profile1', profileName);

    // Test empty name.
    inputBox.value = '';
    assertInput(
        /*value=*/ '', /*invalid=*/ false, /*valueLength=*/ '00');

    // Test name with no emojis, under character limit.
    inputBox.value = '1234567890123456789';
    assertInput(
        /*value=*/ '1234567890123456789', /*invalid=*/ false,
        /*valueLength=*/ '19');

    // Test name with emojis, under character limit.
    inputBox.value = '1234😀5678901234🧟';
    assertInput(
        /*value=*/ '12345678901234', /*invalid=*/ false,
        /*valueLength=*/ '14');

    // Test name with only emojis, under character limit.
    inputBox.value = '😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀';
    assertInput(
        /*value=*/ '', /*invalid=*/ false, /*valueLength=*/ '00');

    // Test name with no emojis, at character limit.
    inputBox.value = '12345678901234567890';
    assertInput(
        /*value=*/ '12345678901234567890', /*invalid=*/ false,
        /*valueLength=*/ '20');

    // Test name with emojis, at character limit.
    inputBox.value = '1234567890123456789🧟';
    assertInput(
        /*value=*/ '1234567890123456789', /*invalid=*/ false,
        /*valueLength=*/ '19');

    // Test name with only emojis, at character limit.
    inputBox.value = '😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀';
    assertInput(
        /*value=*/ '', /*invalid=*/ false, /*valueLength=*/ '00');

    // Test name with no emojis, above character limit.
    inputBox.value = '123456789012345678901';
    assertInput(
        /*value=*/ '12345678901234567890', /*invalid=*/ true,
        /*valueLength=*/ '20');

    // Make sure input is not invalid once its value changes to a string below
    // the character limit. (Simulates the user pressing backspace once they've
    // reached the limit).
    inputBox.value = '1234567890123456789';
    assertInput(
        /*value=*/ '1234567890123456789', /*invalid=*/ false,
        /*valueLength=*/ '19');

    // Test name with emojis, above character limit.
    inputBox.value = '12345678901234567890🧟';
    assertInput(
        /*value=*/ '12345678901234567890', /*invalid=*/ false,
        /*valueLength=*/ '20');

    // Test name with only emojis, above character limit.
    inputBox.value = '😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀😀';
    assertInput(
        /*value=*/ '', /*invalid=*/ false, /*valueLength=*/ '00');

    // Set name with emojis, above character limit
    inputBox.value = '12345678901234567890🧟';
    const doneBtn =
        esimRenameDialog.shadowRoot!.querySelector<HTMLButtonElement>('#done');
    assertTrue(!!doneBtn);
    doneBtn.click();
    await flushTasks();

    const euicc = (await eSimManagerRemote.getAvailableEuiccs()).euiccs[0];
    assertTrue(!!euicc);
    const profile = (await euicc.getProfileList()).profiles[0];
    assertTrue(!!profile);
    const profileProperties = (await profile.getProperties()).properties;

    assertEquals('12345678901234567890', profileProperties.nickname);
  });

  test('Done button is disabled when empty input', async () => {
    eSimManagerRemote.addEuiccForTest(1);
    addEsimCellularNetwork(TEST_CELLULAR_GUID, '1');
    await flushTasks();
    await init();

    const inputBox = esimRenameDialog.shadowRoot!.querySelector<CrInputElement>(
        '#eSimprofileName');
    const doneBtn =
        esimRenameDialog.shadowRoot!.querySelector<HTMLButtonElement>('#done');
    assertTrue(!!inputBox);
    assertTrue(!!doneBtn);

    inputBox.value = 'test';
    assertFalse(doneBtn.disabled);

    inputBox.value = '';
    assertTrue(doneBtn.disabled);

    inputBox.value = 'test2';
    assertFalse(doneBtn.disabled);
  });
});
