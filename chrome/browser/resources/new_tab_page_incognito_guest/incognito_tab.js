// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import {addWebUiListener} from 'chrome://resources/js/cr.js';
import {$} from 'chrome://resources/js/util.js';

window.addEventListener('load', function() {
  // Theme handling
  addWebUiListener('theme-changed', themeData => {
    document.documentElement.setAttribute(
        'hascustombackground', themeData.hasCustomBackground);
    $('incognitothemecss').href =
        'chrome://theme/css/incognito_tab_theme.css?' + Date.now();
  });
  chrome.send('observeThemeChanges');

  // --- TOR TOGGLE ---
  const toggle = document.getElementById('tor-toggle-switch');
  const toggleIcon = document.getElementById('tor-toggle-icon');
  const toggleText = document.getElementById('tor-toggle-text');
  const overlay = document.getElementById('tor-overlay');
  const logsEl = document.getElementById('tor-logs');
  const subtitleEl = document.querySelector('.tor-overlay-subtitle');
  const progressBar = document.getElementById('tor-progress');

  let torEnabled = false;
  let progressInterval = null;
  let torReady = false;
  let fiftySecondsPassed = false;
  let fiftySecondsTimeout = null;

  // Tor is DISABLED by default
  toggle.checked = false;
  updateToggleUI(false, false);

  toggle.addEventListener('change', function() {
    if (toggle.checked) {
      // Enable Tor
      torEnabled = true;
      torReady = false;
      fiftySecondsPassed = false;
      chrome.send('enableTor');
      showLoadingOverlay();
      startProgressBar();
      
      fiftySecondsTimeout = setTimeout(() => {
        fiftySecondsPassed = true;
        checkAndCompleteTor();
      }, 50000);
      
      chrome.send('getTorStatus');
    } else {
      // Disable Tor
      torEnabled = false;
      torReady = false;
      fiftySecondsPassed = false;
      if (fiftySecondsTimeout) {
        clearTimeout(fiftySecondsTimeout);
        fiftySecondsTimeout = null;
      }
      chrome.send('disableTor');
      hideLoadingOverlay();
      stopProgressBar();
      updateToggleUI(false, false);
    }
  });

  function updateToggleUI(isConnecting, isReady) {
    if (isReady) {
      toggleIcon.className = 'tor-toggle-icon connected';
      toggleText.textContent = 'Tor Connected';
    } else if (isConnecting) {
      toggleIcon.className = 'tor-toggle-icon connecting';
      toggleText.textContent = 'Connecting...';
    } else {
      toggleIcon.className = 'tor-toggle-icon';
      toggleText.textContent = 'Tor Disabled';
    }
  }

  function showLoadingOverlay() {
    if (overlay) {
      overlay.classList.add('visible');
    }
    updateToggleUI(true, false);
  }

  function hideLoadingOverlay() {
    if (overlay) {
      overlay.classList.remove('visible');
    }
    if (progressBar) {
      progressBar.style.width = '0%';
    }
  }

  // NEOVEX TOR: 50-second loading bar animation
  function startProgressBar() {
    stopProgressBar();
    const TOTAL_DURATION_MS = 50000;
    const TICK_INTERVAL_MS = 100;
    const startTime = Date.now();

    progressInterval = setInterval(() => {
      const elapsed = Date.now() - startTime;
      const t = Math.min(elapsed / TOTAL_DURATION_MS, 1.0);

      // Ease out up to 95%
      let progress = 95 * (1 - Math.pow(1 - t, 3));

      if (progressBar) {
        progressBar.style.width = progress + '%';
      }
    }, TICK_INTERVAL_MS);
  }

  function stopProgressBar() {
    if (progressInterval) {
      clearInterval(progressInterval);
      progressInterval = null;
    }
  }

  function checkAndCompleteTor() {
    if (torReady && fiftySecondsPassed) {
      stopProgressBar();
      if (progressBar) progressBar.style.width = '100%';

      // Fade out overlay after a moment
      if (subtitleEl) subtitleEl.textContent = 'Connected!';
      setTimeout(() => {
        hideLoadingOverlay();
        updateToggleUI(false, true);
      }, 800);
    }
  }

  // NEOVEX TOR: Listen for real Tor status updates
  addWebUiListener('tor-status-changed', status => {
    if (!torEnabled) return;

    if (status.isReady) {
      torReady = true;
      checkAndCompleteTor();
    } else {
      // Update logs while connecting
      if (logsEl && status.logs) {
        if (logsEl.textContent !== status.logs) {
          logsEl.textContent = status.logs;
          logsEl.scrollTop = logsEl.scrollHeight;
        }
      }
    }
  });
});
