// NEOVEX DOWNLOADS: Category sidebar interactivity
// The sidebar DOM and CSS are in downloads.html.
// This module handles click events and download item filtering.

const CATEGORY_EXTENSIONS: Record<string, string[]> = {
  'Images': ['jpg','jpeg','png','gif','webp','svg','bmp','ico','tiff','raw','heic','avif'],
  'Documents': ['pdf','docx','doc','pptx','ppt','xlsx','xls','txt','md','csv','odt','ods','odp','rtf','epub'],
  'Videos': ['mp4','mkv','avi','mov','wmv','flv','webm','m4v','3gp','mpeg'],
  'Audio': ['mp3','wav','flac','aac','ogg','wma','m4a','opus','aiff'],
  'Archives': ['zip','rar','7z','tar','gz','bz2','xz','iso','cab'],
  'Programs': ['exe','msi','apk','dmg','pkg','deb','run','bat','sh','ps1'],
  'Code': ['js','ts','py','cpp','c','h','java','cs','html','css','json','xml','php','rb','go','rs'],
};

let activeCategory = 'All';

function getExtension(filename: string): string {
  if (!filename) return '';
  const idx = filename.lastIndexOf('.');
  if (idx < 0) return '';
  return filename.substring(idx + 1).toLowerCase();
}

function getCategoryForFile(filename: string): string {
  const ext = getExtension(filename);
  if (!ext) return 'Other';
  for (const cat in CATEGORY_EXTENSIONS) {
    const exts = CATEGORY_EXTENSIONS[cat];
    if (exts && exts.includes(ext)) return cat;
  }
  return 'Other';
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
function getDownloadItems(): any[] {
  const manager = document.querySelector('downloads-manager');
  if (!manager || !manager.shadowRoot) return [];

  // Try cr-infinite-list shadow DOM first
  const list = manager.shadowRoot.querySelector('#downloadsList');
  if (list) {
    if (list.shadowRoot) {
      const items = list.shadowRoot.querySelectorAll('downloads-item');
      if (items.length > 0) return Array.from(items);
    }
    const lightItems = list.querySelectorAll('downloads-item');
    if (lightItems.length > 0) return Array.from(lightItems);
  }

  // Fallback: direct query on manager shadow root
  return Array.from(
      manager.shadowRoot.querySelectorAll('downloads-item'));
}

// eslint-disable-next-line @typescript-eslint/no-explicit-any
function getFilenameFromItem(item: any): string {
  if (!item) return '';
  // Lit data binding
  if (item.data && item.data.fileName) {
    return item.data.fileName;
  }
  // Fallback: read from shadow DOM
  if (item.shadowRoot) {
    const nameEl = item.shadowRoot.querySelector('#name') ||
                   item.shadowRoot.querySelector('.name');
    if (nameEl && nameEl.textContent) {
      return nameEl.textContent.trim();
    }
  }
  return '';
}

function countByCategory(): Record<string, number> {
  const items = getDownloadItems();
  const counts: Record<string, number> = {'All': items.length, 'Other': 0};
  for (const cat in CATEGORY_EXTENSIONS) {
    counts[cat] = 0;
  }
  for (const item of items) {
    const cat = getCategoryForFile(getFilenameFromItem(item));
    counts[cat] = (counts[cat] || 0) + 1;
  }
  return counts;
}

function applyFilter() {
  const items = getDownloadItems();
  for (const item of items) {
    const el = item as HTMLElement;
    const cat = getCategoryForFile(getFilenameFromItem(item));
    if (activeCategory === 'All' || activeCategory === cat) {
      el.style.display = '';
    } else {
      el.style.display = 'none';
    }
  }
  updateCounts();
}

function updateCounts() {
  const counts = countByCategory();
  const sidebar = document.getElementById('neovex-sidebar');
  if (!sidebar) return;
  for (const cat in counts) {
    const el = sidebar.querySelector('[data-count="' + cat + '"]');
    if (el) {
      el.textContent = String(counts[cat]);
    }
  }
}

function switchCategory(category: string) {
  activeCategory = category;
  const sidebar = document.getElementById('neovex-sidebar');
  if (!sidebar) return;
  sidebar.querySelectorAll('.nvx-cat').forEach((btn) => {
    const el = btn as HTMLElement;
    el.classList.toggle(
        'active', el.dataset['category'] === category);
  });
  applyFilter();
}

function setupSidebar() {
  const sidebar = document.getElementById('neovex-sidebar');
  if (!sidebar) return;
  sidebar.querySelectorAll('.nvx-cat').forEach((btn) => {
    btn.addEventListener('click', () => {
      const cat = (btn as HTMLElement).dataset['category'];
      if (cat) switchCategory(cat);
    });
  });
}

function observeChanges() {
  const manager = document.querySelector('downloads-manager');
  if (!manager || !manager.shadowRoot) return;

  const observer = new MutationObserver(() => {
    applyFilter();
  });

  observer.observe(manager.shadowRoot, {
    childList: true,
    subtree: true,
  });

  const list = manager.shadowRoot.querySelector('#downloadsList');
  if (list && list.shadowRoot) {
    observer.observe(list.shadowRoot, {
      childList: true,
      subtree: true,
    });
  }
}

function neovexDownloadsInit() {
  setupSidebar();

  // Wait for downloads-manager to be ready, then start filtering
  const checkReady = window.setInterval(() => {
    const manager = document.querySelector('downloads-manager');
    if (manager && manager.shadowRoot) {
      const list = manager.shadowRoot.querySelector('#downloadsList');
      if (list) {
        window.clearInterval(checkReady);
        window.setTimeout(applyFilter, 300);
        window.setTimeout(applyFilter, 1000);
        window.setTimeout(applyFilter, 2500);
        observeChanges();
      }
    }
  }, 150);
  window.setTimeout(() => window.clearInterval(checkReady), 20000);
}

// Run when DOM is ready
if (document.readyState === 'loading') {
  document.addEventListener('DOMContentLoaded', neovexDownloadsInit);
} else {
  neovexDownloadsInit();
}
