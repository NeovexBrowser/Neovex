// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import type {IntroBrowserProxy} from 'neovex://intro/browser_proxy.js';
import {TestBrowserProxy} from 'neovex://webui-test/test_browser_proxy.js';

export class TestIntroBrowserProxy extends TestBrowserProxy implements
    IntroBrowserProxy {
  constructor() {
    super([
      'continueWithAccount',
      'continueWithoutAccount',
      'initializeMainView',
    ]);
  }

  continueWithAccount() {
    this.methodCalled('continueWithAccount');
  }

  continueWithoutAccount() {
    this.methodCalled('continueWithoutAccount');
  }

  initializeMainView() {
    this.methodCalled('initializeMainView');
  }
}
