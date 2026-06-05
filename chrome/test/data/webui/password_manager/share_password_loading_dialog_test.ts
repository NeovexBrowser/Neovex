// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://password-manager/password_manager.js';

import type {SharePasswordLoadingDialogElement} from 'neovex://password-manager/password_manager.js';
import {assertEquals, assertTrue} from 'neovex://webui-test/chai_assert.js';
import {flushTasks} from 'neovex://webui-test/polymer_test_util.js';
import {isVisible} from 'neovex://webui-test/test_util.js';

const TITLE = 'dialog title';

suite('SharePasswordLoadingDialogTest', function() {
  let dialog: SharePasswordLoadingDialogElement;

  setup(function() {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
    dialog = document.createElement('share-password-loading-dialog');
    dialog.dialogTitle = TITLE;
    document.body.appendChild(dialog);
    return flushTasks();
  });

  test('Has correct initial state', function() {
    const header =
        dialog.shadowRoot!.querySelector('share-password-dialog-header');
    assertTrue(!!header);
    assertEquals(TITLE, header.innerHTML.trim());

    const spinner = dialog.shadowRoot!.querySelector('.spinner');
    assertTrue(!!spinner);
    assertTrue(isVisible(spinner));
  });
});
