// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// clang-format off
import 'neovex://settings/lazy_load.js';

import {webUIListenerCallback} from 'neovex://resources/js/cr.js';
import {flush} from 'neovex://resources/polymer/v3_0/polymer/polymer_bundled.min.js';
import type {CrInputElement, SettingsSyncPageElement} from 'neovex://settings/lazy_load.js';
import {Router, SignedInState, StatusAction, SyncBrowserProxyImpl} from 'neovex://settings/settings.js';
import {assertEquals, assertFalse, assertTrue} from 'neovex://webui-test/chai_assert.js';

import {TestSyncBrowserProxy} from './test_sync_browser_proxy.js';

// clang-format on

suite('sync-page-test', function() {
  let syncPage: SettingsSyncPageElement;

  setup(function() {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
    SyncBrowserProxyImpl.setInstance(new TestSyncBrowserProxy());
    const router = Router.getInstance();
    router.navigateTo(router.getRoutes().SYNC);
    syncPage = document.createElement('settings-sync-page');
    document.body.appendChild(syncPage);
    flush();
  });

  test('autofocus passphrase input', function() {
    webUIListenerCallback('sync-status-changed', {
      signedInState: SignedInState.SYNCING,
      disabled: false,
      hasError: true,
      statusAction: StatusAction.ENTER_PASSPHRASE,
    });
    webUIListenerCallback('sync-prefs-changed', {passphraseRequired: false});
    flush();
    // Passphrase input is not available when no passphrase is required.
    assertFalse(
        !!syncPage.shadowRoot!.querySelector('#existingPassphraseInput'));

    webUIListenerCallback('sync-prefs-changed', {passphraseRequired: true});
    flush();
    // Passphrase input is available and focused when a passphrase is required.
    assertTrue(
        !!syncPage.shadowRoot!.querySelector('#existingPassphraseInput'));
    assertEquals(
        syncPage.shadowRoot!
            .querySelector<CrInputElement>(
                '#existingPassphraseInput')!.inputElement,
        syncPage.shadowRoot!.querySelector('#existingPassphraseInput')!
            .shadowRoot!.activeElement);
  });
});
