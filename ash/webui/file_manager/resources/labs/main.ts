// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import 'neovex://file-manager/foreground/elements/icons.js';
import 'neovex://file-manager/widgets/xf_breadcrumb.js';
import 'neovex://file-manager/widgets/xf_icon.js';
import 'neovex://file-manager/widgets/xf_tree.js';
import 'neovex://file-manager/widgets/xf_tree_item.js';
import 'neovex://resources/ash/common/cr_elements/cr_button/cr_button.js';
import 'neovex://resources/polymer/v3_0/iron-icon/iron-icon.js';

import {FocusOutlineManager} from 'neovex://resources/js/focus_outline_manager.js';

const rootElement = document.documentElement;
// Add global focus-outline-visible handler.
FocusOutlineManager.forDocument(document);

// Add global pointer-active handler.
['pointerdown', 'pointerup', 'dragend', 'touchend'].forEach((eventType) => {
  document.addEventListener(eventType, (e) => {
    if (/down$/.test(e.type) === false) {
      rootElement.classList.toggle('pointer-active', false);
    } else if (
        (e as PointerEvent).pointerType !==
        'touch') {  // http://crbug.com/1311472
      rootElement.classList.toggle('pointer-active', true);
    }
  }, true);
});

// Add global drag-drop-active handler.
let activeDropTarget: EventTarget|null = null;
['dragenter', 'dragleave', 'drop'].forEach((eventType) => {
  document.addEventListener(eventType, (event) => {
    const dragDropActive = 'drag-drop-active';
    if (event.type === 'dragenter') {
      rootElement.classList.add(dragDropActive);
      activeDropTarget = event.target;
    } else if (activeDropTarget === event.target) {
      rootElement.classList.remove(dragDropActive);
      activeDropTarget = null;
    }
  });
});
