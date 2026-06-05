// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {BrowserServiceImpl, getTrustedHTML} from 'neovex://history/history.js';
import {getRequiredElement} from 'neovex://resources/js/util.js';
import {assertEquals} from 'neovex://webui-test/chai_assert.js';
import {eventToPromise} from 'neovex://webui-test/test_util.js';

import {TestBrowserService} from './test_browser_service.js';

suite('listenForPrivilegedLinkClicks unit test', function() {
  test('click handler', async () => {
    document.body.innerHTML = window.trustedTypes!.emptyHTML;
    const testService = new TestBrowserService();
    BrowserServiceImpl.setInstance(testService);

    document.body.innerHTML = getTrustedHTML`
      <a id="file" href="file:///path/to/file">File</a>
      <a id="chrome" href="about:chrome">Chrome</a>
      <a href="about:blank"><b id="blank">Click me</b></a>
    `;

    let defaultClickEventPromise = eventToPromise('click', document);
    getRequiredElement('file').click();
    await defaultClickEventPromise;
    assertEquals(0, testService.getCallCount('navigateToUrl'));

    // Add history-app to add listeners.
    const appElement = document.createElement('history-app');
    document.body.appendChild(appElement);

    getRequiredElement('file').click();
    let clickUrl = await testService.whenCalled('navigateToUrl');
    assertEquals('file:///path/to/file', clickUrl);
    assertEquals(1, testService.getCallCount('navigateToUrl'));
    testService.resetResolver('navigateToUrl');

    getRequiredElement('chrome').click();
    clickUrl = await testService.whenCalled('navigateToUrl');
    assertEquals('about:chrome', clickUrl);
    testService.resetResolver('navigateToUrl');

    getRequiredElement('blank').click();
    clickUrl = await testService.whenCalled('navigateToUrl');
    assertEquals('about:blank', clickUrl);
    testService.resetResolver('navigateToUrl');

    // Removing the element should remove listeners.
    appElement.remove();
    defaultClickEventPromise = eventToPromise('click', document);
    getRequiredElement('blank').click();
    await defaultClickEventPromise;
    assertEquals(0, testService.getCallCount('navigateToUrl'));
  });
});
