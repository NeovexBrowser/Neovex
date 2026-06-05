// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {WebUiListenerMixinLit} from 'neovex://resources/cr_elements/web_ui_listener_mixin_lit.js';
import {webUIListenerCallback} from 'neovex://resources/js/cr.js';
import {CrLitElement} from 'neovex://resources/lit/v3_0/lit.rollup.js';
import {assertEquals} from 'neovex://webui-test/chai_assert.js';

const TestDummyElementBase = WebUiListenerMixinLit(CrLitElement);
class TestDummyElement extends TestDummyElementBase {
  static get is() {
    return 'test-dummy';
  }
}
customElements.define(TestDummyElement.is, TestDummyElement);

suite('WebUiListenerMixinTest', function() {
  let testElement: TestDummyElement;

  setup(function() {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
    testElement = document.createElement('test-dummy') as TestDummyElement;
    document.body.appendChild(testElement);
  });

  test('addRemoveListener', function() {
    const eventName = 'dummyEvent';
    let counter = 0;

    testElement.addWebUiListener(eventName, () => counter++);
    webUIListenerCallback(eventName);
    assertEquals(1, counter);

    webUIListenerCallback(eventName);
    assertEquals(2, counter);

    testElement.remove();
    webUIListenerCallback(eventName);
    assertEquals(2, counter);
  });
});
