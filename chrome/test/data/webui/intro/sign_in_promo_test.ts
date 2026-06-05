// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://intro/sign_in_promo.js';

import {IntroBrowserProxyImpl} from 'neovex://intro/browser_proxy.js';
import type {SignInPromoElement} from 'neovex://intro/sign_in_promo.js';
import {webUIListenerCallback} from 'neovex://resources/js/cr.js';
import {loadTimeData} from 'neovex://resources/js/load_time_data.js';
import {assertEquals, assertTrue} from 'neovex://webui-test/chai_assert.js';
import {microtasksFinished} from 'neovex://webui-test/test_util.js';

import {TestIntroBrowserProxy} from './test_intro_browser_proxy.js';

function checkSignInButtons(element: SignInPromoElement, disabled: boolean) {
  assertEquals(element.$.acceptSignInButton.disabled, disabled);
  assertEquals(element.$.declineSignInButton.disabled, disabled);
}

suite('SignInPromoTest', function() {
  let signInPromoElement: SignInPromoElement;
  let testBrowserProxy: TestIntroBrowserProxy;

  setup(function() {
    testBrowserProxy = new TestIntroBrowserProxy();
    IntroBrowserProxyImpl.setInstance(testBrowserProxy);
  });

  teardown(function() {
    signInPromoElement.remove();
  });

  suite('NonManagedDevice', function() {
    setup(function() {
      loadTimeData.overrideValues({
        isDeviceManaged: false,
      });

      document.body.innerHTML = window.trustedTypes!.emptyHTML;
      signInPromoElement = document.createElement('sign-in-promo');
      document.body.appendChild(signInPromoElement);
    });

    test('accept sign-in button clicked', async function() {
      checkSignInButtons(signInPromoElement, false);
      assertEquals(testBrowserProxy.getCallCount('continueWithAccount'), 0);
      signInPromoElement.$.acceptSignInButton.click();
      await microtasksFinished();
      checkSignInButtons(signInPromoElement, true);
      assertEquals(testBrowserProxy.getCallCount('continueWithAccount'), 1);
    });

    test('decline sign-in button clicked', async function() {
      checkSignInButtons(signInPromoElement, false);
      assertEquals(testBrowserProxy.getCallCount('continueWithoutAccount'), 0);
      signInPromoElement.$.declineSignInButton.click();
      await microtasksFinished();
      checkSignInButtons(signInPromoElement, true);
      assertEquals(testBrowserProxy.getCallCount('continueWithoutAccount'), 1);
    });

    test('"reset-intro-buttons" event resets buttons', async function() {
      checkSignInButtons(signInPromoElement, false);
      signInPromoElement.$.acceptSignInButton.click();
      await microtasksFinished();
      checkSignInButtons(signInPromoElement, true);
      webUIListenerCallback('reset-intro-buttons');
      await microtasksFinished();
      checkSignInButtons(signInPromoElement, false);
    });
  });

  suite('ManagedDevice', function() {
    setup(function() {
      loadTimeData.overrideValues({
        isDeviceManaged: true,
      });

      document.body.innerHTML = window.trustedTypes!.emptyHTML;
      signInPromoElement = document.createElement('sign-in-promo');
      document.body.appendChild(signInPromoElement);
    });

    test('buttons are disabled if disclaimer is empty', async function() {
      checkSignInButtons(signInPromoElement, true);
      assertTrue(
          signInPromoElement.$.disclaimerText.textContent.trim().length === 0);

      webUIListenerCallback(
          'managed-device-disclaimer-updated', 'managedDeviceDisclaimer');
      await microtasksFinished();
      assertTrue(
          signInPromoElement.$.disclaimerText.textContent.trim().length !== 0);
      checkSignInButtons(signInPromoElement, false);
    });
  });
});
