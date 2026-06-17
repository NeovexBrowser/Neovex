// Copyright 2026 Neovex Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {loadTimeData} from 'chrome://resources/js/load_time_data.js';

document.addEventListener('DOMContentLoaded', () => {
    let version = "v1.0.0";
    try {
        if (loadTimeData && loadTimeData.valueExists('neovexVersion')) {
            version = "v" + loadTimeData.getString('neovexVersion');
        }
    } catch (e) {
        console.warn("Failed to load version string", e);
    }

    const badge = document.getElementById('versionBadge');
    if (badge) {
        badge.textContent = version;
    }
});
