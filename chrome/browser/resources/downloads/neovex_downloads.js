// NEOVEX DOWNLOADS: Category sidebar overlay for chrome://downloads
// This script injects a category sidebar and filters the existing
// downloads list by file extension categories.

(function() {
  'use strict';

  // ===== Category definitions =====
  const CATEGORIES = {
    'All':        { icon: '📋', extensions: null },
    'Images':     { icon: '🖼️', extensions: ['jpg','jpeg','png','gif','webp','svg','bmp','ico','tiff','raw','heic','avif'] },
    'Documents':  { icon: '📄', extensions: ['pdf','docx','doc','pptx','ppt','xlsx','xls','txt','md','csv','odt','ods','odp','rtf','epub'] },
    'Videos':     { icon: '🎬', extensions: ['mp4','mkv','avi','mov','wmv','flv','webm','m4v','3gp','mpeg'] },
    'Audio':      { icon: '🎵', extensions: ['mp3','wav','flac','aac','ogg','wma','m4a','opus','aiff'] },
    'Archives':   { icon: '🗜️', extensions: ['zip','rar','7z','tar','gz','bz2','xz','iso','cab'] },
    'Programs':   { icon: '⚙️', extensions: ['exe','msi','apk','dmg','pkg','deb','run','bat','sh','ps1'] },
    'Code':       { icon: '💻', extensions: ['js','ts','py','cpp','c','h','java','cs','html','css','json','xml','php','rb','go','rs'] },
    'Other':      { icon: '📦', extensions: [] }
  };

  let activeCategory = 'All';
  let sidebarEl = null;
  let badgeEl = null;

  // ===== Utility: get extension from filename =====
  function getExtension(filename) {
    if (!filename) return '';
    const parts = filename.split('.');
    if (parts.length < 2) return '';
    return parts.pop().toLowerCase();
  }

  // ===== Utility: determine category for a file =====
  function getCategoryForFile(filename) {
    const ext = getExtension(filename);
    if (!ext) return 'Other';
    for (const [cat, data] of Object.entries(CATEGORIES)) {
      if (cat === 'All' || cat === 'Other') continue;
      if (data.extensions && data.extensions.includes(ext)) return cat;
    }
    return 'Other';
  }

  // ===== Get all downloads-item elements from shadow DOM =====
  function getDownloadItems() {
    const manager = document.querySelector('downloads-manager');
    if (!manager || !manager.shadowRoot) return [];

    const list = manager.shadowRoot.querySelector('#downloadsList');
    if (!list || !list.shadowRoot) return [];

    // cr-infinite-list renders items in its shadow DOM
    const items = list.shadowRoot.querySelectorAll('downloads-item');
    if (items.length > 0) return Array.from(items);

    // Fallback: items might be in the light DOM of the list
    const lightItems = list.querySelectorAll('downloads-item');
    if (lightItems.length > 0) return Array.from(lightItems);

    // Another fallback: check manager's shadow root directly
    return Array.from(manager.shadowRoot.querySelectorAll('downloads-item'));
  }

  // ===== Get filename from a downloads-item element =====
  function getFilenameFromItem(item) {
    if (!item) return '';
    // The item has a .data property from Lit binding
    if (item.data && item.data.fileName) {
      return item.data.fileName;
    }
    // Fallback: try reading from shadow DOM text content
    if (item.shadowRoot) {
      const nameEl = item.shadowRoot.querySelector('#name') ||
                     item.shadowRoot.querySelector('[id*="file-name"]') ||
                     item.shadowRoot.querySelector('.name');
      if (nameEl) return nameEl.textContent.trim();
    }
    return '';
  }

  // ===== Count items per category =====
  function countByCategory(items) {
    const counts = {};
    for (const cat of Object.keys(CATEGORIES)) {
      counts[cat] = 0;
    }
    counts['All'] = items.length;

    for (const item of items) {
      const filename = getFilenameFromItem(item);
      const cat = getCategoryForFile(filename);
      counts[cat] = (counts[cat] || 0) + 1;
    }
    return counts;
  }

  // ===== Filter: show/hide items based on active category =====
  function applyFilter() {
    const items = getDownloadItems();
    const counts = countByCategory(items);

    for (const item of items) {
      const filename = getFilenameFromItem(item);
      const cat = getCategoryForFile(filename);

      if (activeCategory === 'All') {
        item.style.display = '';
      } else if (activeCategory === cat) {
        item.style.display = '';
      } else {
        item.style.display = 'none';
      }
    }

    updateSidebarCounts(counts);
    updateBadge();
  }

  // ===== Build the sidebar DOM =====
  function buildSidebar() {
    if (document.getElementById('neovex-sidebar')) return;

    sidebarEl = document.createElement('aside');
    sidebarEl.id = 'neovex-sidebar';

    // Header
    const header = document.createElement('div');
    header.className = 'sidebar-header';
    header.innerHTML = `
      <div class="logo-icon">⬇</div>
      <h1>Downloads</h1>
    `;
    sidebarEl.appendChild(header);

    // Category list
    const nav = document.createElement('nav');
    nav.className = 'category-list';
    nav.id = 'neovex-cat-list';

    for (const [name, data] of Object.entries(CATEGORIES)) {
      const item = document.createElement('div');
      item.className = 'category-item' + (name === activeCategory ? ' active' : '');
      item.dataset.category = name;
      item.innerHTML = `
        <span class="cat-icon">${data.icon}</span>
        <span class="cat-name">${name}</span>
        <span class="cat-count" data-cat-count="${name}">0</span>
      `;
      item.addEventListener('click', () => switchCategory(name));
      nav.appendChild(item);
    }
    sidebarEl.appendChild(nav);

    // Footer buttons
    const footer = document.createElement('div');
    footer.className = 'sidebar-footer';

    const openBtn = document.createElement('button');
    openBtn.textContent = '📂 Open Folder';
    openBtn.addEventListener('click', () => {
      // Trigger the existing "Open downloads folder" button in toolbar
      const manager = document.querySelector('downloads-manager');
      if (manager && manager.shadowRoot) {
        const toolbar = manager.shadowRoot.querySelector('#toolbar');
        if (toolbar && toolbar.shadowRoot) {
          const crToolbar = toolbar.shadowRoot.querySelector('cr-toolbar');
          if (crToolbar && crToolbar.shadowRoot) {
            // Use Mojo handler directly
            try {
              const proxy = manager.mojoHandler_ ||
                (manager.constructor && manager.constructor.prototype);
              // Fallback: simulate keyboard shortcut or click
            } catch(e) {}
          }
        }
      }
      // Direct Mojo call via BrowserProxy
      try {
        const bp = window.__neovexBrowserProxy;
        if (bp) bp.handler.openDownloadsFolderRequiringGesture();
      } catch(e) {}
    });
    footer.appendChild(openBtn);

    sidebarEl.appendChild(footer);

    // Insert sidebar as first child of body
    document.body.insertBefore(sidebarEl, document.body.firstChild);
  }

  // ===== Build filter badge (shown inside the main content area) =====
  function buildBadge() {
    if (document.getElementById('neovex-cat-badge')) return;

    badgeEl = document.createElement('div');
    badgeEl.id = 'neovex-cat-badge';
    badgeEl.innerHTML = `
      <span class="badge-icon"></span>
      <span class="badge-text"></span>
      <span class="badge-clear" title="Show all">✕</span>
    `;
    badgeEl.querySelector('.badge-clear').addEventListener('click', () => {
      switchCategory('All');
    });

    // Insert badge into the manager's main container
    const manager = document.querySelector('downloads-manager');
    if (manager && manager.shadowRoot) {
      const mainContainer = manager.shadowRoot.querySelector('#mainContainer');
      if (mainContainer) {
        mainContainer.insertBefore(badgeEl, mainContainer.firstChild);
      }
    }
  }

  // ===== Update sidebar item counts =====
  function updateSidebarCounts(counts) {
    if (!sidebarEl) return;
    for (const [cat, count] of Object.entries(counts)) {
      const el = sidebarEl.querySelector(`[data-cat-count="${cat}"]`);
      if (el) el.textContent = count;
    }
  }

  // ===== Update the filter badge =====
  function updateBadge() {
    if (!badgeEl) return;
    if (activeCategory === 'All') {
      badgeEl.classList.remove('visible');
    } else {
      badgeEl.classList.add('visible');
      const data = CATEGORIES[activeCategory];
      badgeEl.querySelector('.badge-icon').textContent = data ? data.icon : '';
      badgeEl.querySelector('.badge-text').textContent =
        `Showing: ${activeCategory}`;
    }
  }

  // ===== Switch to a different category =====
  function switchCategory(category) {
    activeCategory = category;

    // Update active state in sidebar
    if (sidebarEl) {
      sidebarEl.querySelectorAll('.category-item').forEach(el => {
        el.classList.toggle('active', el.dataset.category === category);
      });
    }

    applyFilter();
  }

  // ===== Try to capture BrowserProxy for Open Folder =====
  function captureBrowserProxy() {
    try {
      // The BrowserProxy is a singleton
      const modUrl = 'chrome://downloads/browser_proxy.js';
      import(modUrl).then(mod => {
        if (mod && mod.BrowserProxy) {
          window.__neovexBrowserProxy = mod.BrowserProxy.getInstance();
        }
      }).catch(() => {});
    } catch(e) {}
  }

  // ===== MutationObserver to react to download list changes =====
  function observeChanges() {
    const manager = document.querySelector('downloads-manager');
    if (!manager || !manager.shadowRoot) return;

    // Observe the manager's shadow root for child changes
    const observer = new MutationObserver(() => {
      applyFilter();
    });

    observer.observe(manager.shadowRoot, {
      childList: true,
      subtree: true,
      attributes: false,
    });

    // Also observe the infinite list if available
    const list = manager.shadowRoot.querySelector('#downloadsList');
    if (list && list.shadowRoot) {
      observer.observe(list.shadowRoot, {
        childList: true,
        subtree: true,
      });
    }
  }

  // ===== Initialize everything =====
  function init() {
    buildSidebar();

    // Wait for the downloads-manager to be ready
    const checkReady = setInterval(() => {
      const manager = document.querySelector('downloads-manager');
      if (manager && manager.shadowRoot) {
        const list = manager.shadowRoot.querySelector('#downloadsList');
        if (list) {
          clearInterval(checkReady);

          // Build badge (needs to be inside shadow DOM)
          buildBadge();

          // Initial filter application
          setTimeout(applyFilter, 500);
          setTimeout(applyFilter, 1500);
          setTimeout(applyFilter, 3000);

          // Start observing for dynamic changes
          observeChanges();

          // Capture browser proxy for folder open
          captureBrowserProxy();
        }
      }
    }, 200);

    // Safety timeout: stop checking after 15 seconds
    setTimeout(() => {
      clearInterval(checkReady);
    }, 15000);
  }

  // ===== Start when DOM is ready =====
  if (document.readyState === 'loading') {
    document.addEventListener('DOMContentLoaded', init);
  } else {
    init();
  }

})();
