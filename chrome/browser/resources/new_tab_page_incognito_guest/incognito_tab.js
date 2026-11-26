// Copyright 2026 Rhenium Authors
// Incognito tab UI controller (minimal NTP style with Tor support and shared NTP background)

import {addWebUiListener} from 'chrome://resources/js/cr.js';

window.addEventListener('load', function() {
  // ── CLOCK ──
  function updateClock() {
    const d = new Date();
    const clockEl = document.getElementById('clock');
    if (clockEl) {
      clockEl.textContent =
          String(d.getHours()).padStart(2, '0') + ':' +
          String(d.getMinutes()).padStart(2, '0');
    }
  }
  updateClock();
  setInterval(updateClock, 10000);

  // ── SEARCH ──
  const searchBar = document.getElementById('searchBar');
  const searchInput = document.getElementById('searchInput');
  const searchBtn = document.getElementById('searchBtn');

  if (searchBar && searchInput) {
    searchBar.addEventListener('click', () => searchInput.focus());
  }

  function doSearch() {
    if (!searchInput) return;
    const q = searchInput.value.trim();
    if (q) {
      // In incognito mode, use DuckDuckGo as default private search or search engine query
      window.location.href = 'https://duckduckgo.com/?q=' + encodeURIComponent(q);
    }
  }

  if (searchInput) {
    searchInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter') doSearch();
    });
  }
  if (searchBtn) {
    searchBtn.addEventListener('click', doSearch);
  }

  // ── THEME SUPPORT & BACKGROUND ──
  function updateTheme() {
    const isLight = window.matchMedia && window.matchMedia('(prefers-color-scheme: light)').matches;
    if (isLight) {
      document.documentElement.classList.add('light-mode');
    } else {
      document.documentElement.classList.remove('light-mode');
    }
    const bgEl = document.getElementById('bgCustom');
    if (bgEl) {
      const bgUrl = isLight ? bgEl.dataset.light : bgEl.dataset.dark;
      if (bgUrl) {
        bgEl.style.backgroundImage = `url("${bgUrl}")`;
      }
    }
  }
  updateTheme();
  if (window.matchMedia) {
    window.matchMedia('(prefers-color-scheme: light)').addEventListener('change', updateTheme);
  }

  // ── TOR TOGGLE & STATUS ──
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

  if (toggle) {
    toggle.checked = false;
    updateToggleUI(false, false);

    // Request initial Tor status
    try {
      chrome.send('getTorStatus');
    } catch (e) {}

    toggle.addEventListener('change', function() {
      if (toggle.checked) {
        torEnabled = true;
        torReady = false;
        fiftySecondsPassed = false;
        try { chrome.send('enableTor'); } catch (e) {}
        showLoadingOverlay();
        startProgressBar();

        fiftySecondsTimeout = setTimeout(() => {
          fiftySecondsPassed = true;
          checkAndCompleteTor();
        }, 50000);

        try { chrome.send('getTorStatus'); } catch (e) {}
      } else {
        torEnabled = false;
        torReady = false;
        fiftySecondsPassed = false;
        if (fiftySecondsTimeout) {
          clearTimeout(fiftySecondsTimeout);
          fiftySecondsTimeout = null;
        }
        try { chrome.send('disableTor'); } catch (e) {}
        hideLoadingOverlay();
        stopProgressBar();
        updateToggleUI(false, false);
      }
    });
  }

  function updateToggleUI(isConnecting, isReady) {
    if (!toggleIcon || !toggleText) return;
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
    if (overlay) overlay.classList.add('visible');
    updateToggleUI(true, false);
  }

  function hideLoadingOverlay() {
    if (overlay) overlay.classList.remove('visible');
    if (progressBar) progressBar.style.width = '0%';
  }

  function startProgressBar() {
    stopProgressBar();
    const TOTAL_DURATION_MS = 50000;
    const TICK_INTERVAL_MS = 100;
    const startTime = Date.now();

    progressInterval = setInterval(() => {
      const elapsed = Date.now() - startTime;
      const t = Math.min(elapsed / TOTAL_DURATION_MS, 1.0);
      const progress = 95 * (1 - Math.pow(1 - t, 3));
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
      if (subtitleEl) subtitleEl.textContent = 'Connected!';
      setTimeout(() => {
        hideLoadingOverlay();
        updateToggleUI(false, true);
      }, 800);
    }
  }

  addWebUiListener('tor-status-changed', status => {
    if (!status) return;
    if (!torEnabled && (status.isReady || status.isRunning)) {
      torEnabled = true;
      if (toggle) toggle.checked = true;
      fiftySecondsPassed = true;
      if (status.isReady) {
        torReady = true;
        updateToggleUI(false, true);
        hideLoadingOverlay();
      } else {
        showLoadingOverlay();
      }
    }

    if (torEnabled && !status.isReady && !status.isRunning && status.logs === "Waiting for Tor to start...") {
      torEnabled = false;
      torReady = false;
      if (toggle) toggle.checked = false;
      updateToggleUI(false, false);
      hideLoadingOverlay();
      stopProgressBar();
    }

    if (!torEnabled) return;

    if (status.isReady) {
      torReady = true;
      checkAndCompleteTor();
    } else {
      if (logsEl && status.logs) {
        if (logsEl.textContent !== status.logs) {
          logsEl.textContent = status.logs;
          logsEl.scrollTop = logsEl.scrollHeight;
        }
      }
    }
  });
});
