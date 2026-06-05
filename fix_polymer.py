import os

files = [
    r"n:\chromium\src\tools\polymer\html_to_wrapper.py",
    r"n:\chromium\src\tools\polymer\css_to_wrapper.py"
]

for fpath in files:
    with open(fpath, "r", encoding="utf-8") as f:
        content = f.read()
    content = content.replace("'chrome:'", "'neovex:'")
    with open(fpath, "w", encoding="utf-8", newline="") as f:
        f.write(content)
