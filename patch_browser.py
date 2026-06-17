import sys

file_path = r'n:\chromium\src\chrome\browser\ui\browser.cc'
with open(file_path, 'r', encoding='utf-8') as f:
    content = f.read()

content = content.replace('#include "chrome/browser/neovex/update/neovex_update_checker.h"', '')
content = content.replace('neovex::NeovexUpdateChecker::GetInstance()->CheckForUpdates();', '// (Old update checker removed)')

with open(file_path, 'w', encoding='utf-8', newline='') as f:
    f.write(content)
print('Patch applied successfully!')
