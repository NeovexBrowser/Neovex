import shutil
import os

path = r"n:\chromium\src\out\Default\gen\ui\webui\resources\tsc\mojo"
if os.path.exists(path):
    shutil.rmtree(path)
    print(f"Deleted {path}")
else:
    print(f"Path does not exist: {path}")
