"""Restore pinned dependencies and project-local Windows build tools."""
import hashlib
import subprocess
import urllib.request
import zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
toolchain=ROOT/'toolchain'; toolchain.mkdir(exist_ok=True)
def fetch_zip(url,filename,destination,sha1=None):
    archive=toolchain/filename
    if not archive.exists():
        print('Downloading',filename,flush=True); urllib.request.urlretrieve(url,archive)
    if sha1:
        digest=hashlib.sha1()
        with archive.open('rb') as f:
            for block in iter(lambda:f.read(1024*1024),b''): digest.update(block)
        if digest.hexdigest()!=sha1: raise RuntimeError('NDK checksum failed')
    with zipfile.ZipFile(archive) as z: z.extractall(destination)
if not (toolchain/'qpm/qpm.exe').exists():
    fetch_zip('https://github.com/QuestPackageManager/QPM.CLI/releases/download/v1.5.11/qpm-windows-x64.zip','qpm.zip',toolchain/'qpm')
if not (toolchain/'ninja/ninja.exe').exists():
    fetch_zip('https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip','ninja.zip',toolchain/'ninja')
if not (toolchain/'android-ndk-r27c/build/cmake/android.toolchain.cmake').exists():
    fetch_zip('https://dl.google.com/android/repository/android-ndk-r27c-windows.zip','android-ndk-r27c-windows.zip',toolchain,'ac5f7762764b1f15341094e148ad4f847d050c38')
(ROOT/'ndkpath.txt').write_text(str(toolchain/'android-ndk-r27c').replace('\\','/')+'\n')
if not (ROOT/'extern.cmake').exists():
    print('Restoring dependencies; details in bootstrap.log',flush=True)
    with (ROOT/'bootstrap.log').open('w') as log:
        subprocess.run([str(toolchain/'qpm/qpm.exe'),'restore'],cwd=ROOT,stdout=log,stderr=subprocess.STDOUT,check=True)
tiny=ROOT/'extern/libs/libtinyxml2.so'
if not tiny.exists(): urllib.request.urlretrieve('https://github.com/MillzyDev/NDK-tinyxml2/releases/download/v10.0.0/libtinyxml2.so',tiny)
print('Build dependencies ready. Run scripts/build.ps1.',flush=True)
