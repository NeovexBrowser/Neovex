// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {html, nothing} from '//resources/lit/v3_0/lit.rollup.js';

import type {CrToolbarElement} from './cr_toolbar.js';

export function getHtml(this: CrToolbarElement) {
  // clang-format off
  return html`
<div id="leftContent">
  <div id="leftSpacer">
    ${this.showMenu ? html`
      <cr-icon-button id="menuButton" class="no-overlap"
          iron-icon="cr20:menu" @click="${this.onMenuClick_}"
          aria-label="${this.menuLabel || nothing}"
          title="${this.menuLabel}">
      </cr-icon-button>` : ''}
    <slot name="product-logo">
      <picture>
        <img id="product-logo"
            src="//resources/images/neovex_logo.svg"
            role="presentation">
      </picture>
    </slot>
    <h1>${this.pageName}</h1>
  </div>
</div>

<div id="centeredContent" ?hidden="${!this.showSearch}">
  <cr-toolbar-search-field id="search" ?narrow="${this.narrow}"
      label="${this.searchPrompt}" clear-label="${this.clearLabel}"
      ?spinner-active="${this.spinnerActive}"
      ?showing-search="${this.showingSearch_}"
      @showing-search-changed="${this.onShowingSearchChanged_}"
      ?autofocus="${this.autofocus}" icon-override="${this.searchIconOverride}"
      input-aria-description="${this.searchInputAriaDescription}">
  </cr-toolbar-search-field>
</div>

<div id="rightContent">
  <div id="rightSpacer">
    <slot></slot>
  </div>
</div>`;
  // clang-format on
}
