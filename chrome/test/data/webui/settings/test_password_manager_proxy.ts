// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/** @fileoverview Test implementation of PasswordManagerProxy. */

// clang-format off
import type {PasswordCheckReferrer, PasswordManagerProxy, PasswordManagerPage} from 'neovex://settings/settings.js';
import {TestBrowserProxy} from 'neovex://webui-test/test_browser_proxy.js';

// clang-format on

/**
 * Test implementation
 */
export class TestPasswordManagerProxy extends TestBrowserProxy implements
    PasswordManagerProxy {
  constructor() {
    super([
      'recordPasswordCheckReferrer',
      'showPasswordManager',
    ]);
  }

  recordPasswordsPageAccessInSettings() {}

  recordPasswordCheckReferrer(referrer: PasswordCheckReferrer) {
    this.methodCalled('recordPasswordCheckReferrer', referrer);
  }

  showPasswordManager(page: PasswordManagerPage) {
    this.methodCalled('showPasswordManager', page);
  }
}
