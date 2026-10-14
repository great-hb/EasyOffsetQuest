"""Package the mod and check ARM64, loader exports, and segment alignment."""
import hashlib
import json
import shutil
import struct
import subprocess
import zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
dist=ROOT/'dist'; dist.mkdir(exist_ok=True)
original=ROOT/'build/libEasyOffsetQuest.so'
if not original.is_file(): raise SystemExit('Compile the mod first.')
manifest=json.loads((ROOT/'mod.json').read_text())
ndk=ROOT/'toolchain/android-ndk-r27c/toolchains/llvm/prebuilt/windows-x86_64/bin'
library=dist/'libEasyOffsetQuest.so'; shutil.copy2(original,library)
subprocess.run([str(ndk/'llvm-strip.exe'),'--strip-unneeded',str(library)],check=True)
binary=library.read_bytes()
assert binary[:6]==b'\x7fELF\x02\x01' and struct.unpack_from('<H',binary,18)[0]==183
phoff=struct.unpack_from('<Q',binary,32)[0]; entsize,count=struct.unpack_from('<HH',binary,54)
alignments=[]
for i in range(count):
    row=struct.unpack_from('<IIQQQQQQ',binary,phoff+i*entsize)
    if row[0]==1: alignments.append(row[7]); assert row[7]>=16384
symbols=subprocess.check_output([str(ndk/'llvm-readelf.exe'),'-Ws',str(library)],text=True)
for symbol in ['setup','late_load']:
    assert any(line.endswith(' '+symbol) and 'GLOBAL' in line and 'UND' not in line for line in symbols.splitlines())
needed=subprocess.check_output([str(ndk/'llvm-readelf.exe'),'-d',str(library)],text=True)
assert '.debug.so' not in needed
libraries=[line.split('[',1)[1].split(']',1)[0] for line in needed.splitlines() if '(NEEDED)' in line]
assert set(libraries)<=set(['libbeatsaber-hook.so','libbsml.so','libcustom-types.so','libpaper2_scotland2.so','libsl2.so','libtinyxml2.so','liblog.so','libm.so','libdl.so','libc.so'])
shutil.copy2(ROOT/'mod.json',dist/'mod.json')
qmod=dist/'EasyOffsetQuest.qmod'
with zipfile.ZipFile(qmod,'w',zipfile.ZIP_DEFLATED) as z:
    for p in [library,dist/'mod.json',ROOT/'LICENSE-EasyOffset',ROOT/'THIRD-PARTY-NOTICES.md']: z.write(p,p.name)
with zipfile.ZipFile(qmod) as z:
    assert z.testzip() is None
    assert all(name in z.namelist() for name in json.loads(z.read('mod.json'))['modFiles'])
validation=json.loads((ROOT/'build-tests/results.json').read_text()) if (ROOT/'build-tests/results.json').exists() else {}
report=dict(modId=manifest['id'],version=manifest['version'],gameVersion=manifest['packageVersion'],
            qmodSha256=hashlib.sha256(qmod.read_bytes()).hexdigest(),librarySha256=hashlib.sha256(binary).hexdigest(),
            arm64Verified=True,loadSegmentAlignments=alignments,exportsVerified=['setup','late_load'],
            runtimeLibraries=libraries,validation=validation,gameRuntimeTested=False)
(dist/'BUILD-INFO.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'Packaged {qmod} ({qmod.stat().st_size:,} bytes)'); print(json.dumps(report,indent=2))
