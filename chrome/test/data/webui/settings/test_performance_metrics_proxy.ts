// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import type {BatterySaverModeState, MemorySaverModeAggressiveness, MemorySaverModeExceptionListAction, MemorySaverModeState, PerformanceMetricsProxy} from 'neovex://settings/settings.js';
import {TestBrowserProxy} from 'neovex://webui-test/test_browser_proxy.js';

export class TestPerformanceMetricsProxy extends TestBrowserProxy implements
    PerformanceMetricsProxy {
  constructor() {
    super([
      'recordBatterySaverModeChanged',
      'recordMemorySaverModeChanged',
      'recordMemorySaverModeAggressivenessChanged',
      'recordDiscardRingTreatmentEnabledChanged',
      'recordExceptionListAction',
      'recordPerformanceInterventionToggleButtonChanged',
    ]);
  }

  recordBatterySaverModeChanged(state: BatterySaverModeState) {
    this.methodCalled('recordBatterySaverModeChanged', state);
  }

  recordMemorySaverModeChanged(state: MemorySaverModeState) {
    this.methodCalled('recordMemorySaverModeChanged', state);
  }

  recordMemorySaverModeAggressivenessChanged(
      aggressiveness: MemorySaverModeAggressiveness) {
    this.methodCalled(
        'recordMemorySaverModeAggressivenessChanged', aggressiveness);
  }

  recordDiscardRingTreatmentEnabledChanged(enabled: boolean) {
    this.methodCalled('recordDiscardRingTreatmentEnabledChanged', enabled);
  }

  recordExceptionListAction(action: MemorySaverModeExceptionListAction) {
    this.methodCalled('recordExceptionListAction', action);
  }

  recordPerformanceInterventionToggleButtonChanged(enabled: boolean) {
    this.methodCalled(
        'recordPerformanceInterventionToggleButtonChanged', enabled);
  }
}
