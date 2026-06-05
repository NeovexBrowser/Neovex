// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://os-print/js/destination_row_controller.js';

import {DestinationRowController} from 'neovex://os-print/js/destination_row_controller.js';
import {assertTrue} from 'neovex://webui-test/chromeos/chai_assert.js';

import {resetDataManagersAndProviders} from './test_utils.js';

suite('DestinationRowController', () => {
  let controller: DestinationRowController;

  setup(() => {
    resetDataManagersAndProviders();
    controller = new DestinationRowController();
  });

  teardown(() => resetDataManagersAndProviders());

  // Verify controller is an EventTarget.
  test('controller is an event target', () => {
    assertTrue(
        controller instanceof EventTarget, 'Controller is an event target');
  });
});
