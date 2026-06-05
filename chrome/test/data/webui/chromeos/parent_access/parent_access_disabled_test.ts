// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://webui-test/chromeos/mojo_webui_test_support.js';
import 'neovex://parent-access/parent_access_disabled.js';
import 'neovex://parent-access/strings.m.js';

import {ParentAccessResult} from 'neovex://parent-access/parent_access_ui.mojom-webui.js';
import {resetParentAccessHandlerForTest, setParentAccessUiHandlerForTest} from 'neovex://parent-access/parent_access_ui_handler.js';
import {assertEquals} from 'neovex://webui-test/chai_assert.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';

import {buildExtensionApprovalsParamsWithoutPermissions, clearDocumentBody} from './parent_access_test_utils.js';
import {TestParentAccessUiHandler} from './test_parent_access_ui_handler.js';

suite('ParentAccessDisabledTest', function() {
  setup(function() {
    clearDocumentBody();
  });

  teardown(() => {
    resetParentAccessHandlerForTest();
  });

  test('TestOkButton', async () => {
    // Set up the ParentAccessParams for the extension approvals flow.
    const handler = new TestParentAccessUiHandler();
    handler.setParentAccessParams(
        buildExtensionApprovalsParamsWithoutPermissions(
            /**isDisabled=*/ true));
    setParentAccessUiHandlerForTest(handler);

    // Render ParentAccessDisabled element
    const parentAccessDisabled =
        document.createElement('parent-access-disabled');
    document.body.appendChild(parentAccessDisabled);
    await flushTasks();

    // Assert the disabled result is sent when the OK button is clicked.
    assertEquals(handler.getCallCount('onParentAccessDone'), 0);
    const okButton =
        parentAccessDisabled.shadowRoot!.querySelector<HTMLElement>(
            '.action-button')!;
    okButton.click();
    assertEquals(handler.getCallCount('onParentAccessDone'), 1);
    assertEquals(
        handler.getArgs('onParentAccessDone')[0], ParentAccessResult.kDisabled);
  });
});
