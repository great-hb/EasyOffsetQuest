"""Save the pinned dependency QMODs; adjust generic tinyxml's package target."""
import hashlib
import io
import json
import urllib.request
import zipfile
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
out=ROOT/'dist/dependencies'; out.mkdir(parents=True,exist_ok=True)
upstream=out/'upstream'; upstream.mkdir(exist_ok=True)
records=[]
for dependency in json.loads((ROOT/'mod.json').read_text())['dependencies']:
    path=upstream/(dependency['id']+'.qmod')
    if not path.exists():
        path.write_bytes(urllib.request.urlopen(dependency['downloadIfMissing'],timeout=30).read())
    data=path.read_bytes()
    with zipfile.ZipFile(io.BytesIO(data)) as z:
        manifest=json.loads(z.read('mod.json'))
        assert manifest['id']==dependency['id']
        blobs={name:z.read(name) for name in z.namelist() if not name.endswith('/')}
    changed=False
    if dependency['id']=='tinyxml2':
        # This is a generic XML library; the upstream package still declares
        # Beat Saber 1.29. Its exact original shared library is preserved.
        manifest['packageVersion']='1.40.8_7379'
        blobs['mod.json']=(json.dumps(manifest,indent=2)+'\n').encode()
        changed=True
    destination=out/path.name
    if changed:
        with zipfile.ZipFile(destination,'w',zipfile.ZIP_DEFLATED) as z:
            for name,blob in blobs.items(): z.writestr(name,blob)
        with zipfile.ZipFile(destination) as z:
            assert z.read('libtinyxml2.so')==zipfile.ZipFile(io.BytesIO(data)).read('libtinyxml2.so')
    else: destination.write_bytes(data)
    records.append(dict(id=manifest['id'],version=manifest['version'],packageVersion=manifest.get('packageVersion'),
                        upstreamUrl=dependency['downloadIfMissing'],upstreamSha256=hashlib.sha256(data).hexdigest(),
                        localSha256=hashlib.sha256(destination.read_bytes()).hexdigest(),metadataRepackaged=changed))
(out/'DEPENDENCY-INFO.json').write_text(json.dumps(records,indent=2)+'\n')
print('Dependency packages ready in dist/dependencies (original archives under upstream).')
