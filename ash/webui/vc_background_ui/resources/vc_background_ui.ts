// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import '/strings.m.js';
import 'neovex://resources/ash/common/sea_pen/sea_pen_router_element.js';
import './js/vc_background_app.js';

import {emptyState} from 'neovex://resources/ash/common/sea_pen/sea_pen_state.js';
import {getSeaPenStore} from 'neovex://resources/ash/common/sea_pen/sea_pen_store.js';
import {ColorChangeUpdater} from 'neovex://resources/cr_components/color_change_listener/colors_css_updater.js';

getSeaPenStore().init(emptyState());
ColorChangeUpdater.forDocument().start();
