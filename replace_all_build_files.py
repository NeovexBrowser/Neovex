import os
import glob

# directories to skip
skip_dirs = {'out', 'third_party', '.git', 'build'}

count = 0
for root, dirs, files in os.walk(r"n:\chromium\src"):
    dirs[:] = [d for d in dirs if d not in skip_dirs]
    for file in files:
        if file.endswith('.gn') or file.endswith('.gni') or file.endswith('.py'):
            filepath = os.path.join(root, file)
            try:
                with open(filepath, 'r', encoding='utf-8') as f:
                    content = f.read()
                
                if 'neovex://' in content:
                    new_content = content.replace('neovex://', 'neovex://')
                    with open(filepath, 'w', encoding='utf-8', newline='') as f:
                        f.write(new_content)
                    count += 1
                    print(f"Updated: {filepath}")
            except Exception as e:
                pass

print(f"Total files updated: {count}")
