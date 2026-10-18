"""Check version consistency and validate mod.json against MBF's QMOD schema."""
import argparse
import json
import os
from pathlib import Path, PurePosixPath
import re
import urllib.request
import zipfile

from jsonschema import Draft202012Validator

ROOT = Path(__file__).resolve().parents[1]
# Use the schema that MBF actually embeds, pinned so validation is reproducible.
SCHEMA_URL = (
    'https://raw.githubusercontent.com/Lauriethefish/ModsBeforeFriday/'
    '2d6f84a9f9c9d5a5723866de9c8ea0dfb20b37d4/'
    'mbf-agent/src/mod_man/qmod_schema.json'
)


def normalize_schema(value):
    # MBF embeds legacy fragment-only $id annotations under a 2020-12 header.
    # Python rejects those annotations; remove them without changing constraints.
    if isinstance(value, dict):
        return {key: normalize_schema(item) for key, item in value.items()
                if not (key == '$id' and isinstance(item, str) and item.startswith('#'))}
    if isinstance(value, list):
        return [normalize_schema(item) for item in value]
    return value


def validate_versions(manifest, ref=''):
    version = manifest['version']
    qpm = json.loads((ROOT / 'qpm.json').read_text())
    shared = json.loads((ROOT / 'qpm.shared.json').read_text())
    cmake = (ROOT / 'CMakeLists.txt').read_text()
    versions = {
        'qpm.json': qpm['info']['version'],
        'qpm.shared.json': shared['config']['info']['version'],
    }
    project = re.search(r'project\(EasyOffsetQuest\s+VERSION\s+([^\s)]+)', cmake)
    compiled = re.search(r'\bVERSION="([^"]+)"', cmake)
    if not project or not compiled:
        raise ValueError('Could not read the CMake project and compiled mod versions')
    versions['CMake project'] = project[1]
    versions['CMake VERSION definition'] = compiled[1]
    for source, actual in versions.items():
        if actual != version:
            raise ValueError(f'{source}: {actual} does not match mod.json version {version}')
    if ref.startswith('refs/tags/') and ref != f'refs/tags/v{version}':
        raise ValueError(f'Release tag {ref} does not match mod.json version {version}')
    if manifest.get('modloader') != 'Scotland2':
        raise ValueError('Expected the Scotland2 modloader')
    if manifest.get('packageId') != 'com.beatgames.beatsaber':
        raise ValueError('Expected the Beat Saber Android package')
    return version


def validate_archive(path, manifest):
    with zipfile.ZipFile(path) as archive:
        names = archive.namelist()
        if len(names) != len(set(names)):
            raise ValueError('QMOD contains duplicate filenames')
        for name in names:
            parts = PurePosixPath(name)
            if parts.is_absolute() or '..' in parts.parts or '\\' in name or ':' in name:
                raise ValueError(f'Unsafe QMOD filename: {name}')
        if archive.testzip() is not None:
            raise ValueError('QMOD failed its ZIP CRC check')
        if json.loads(archive.read('mod.json')) != manifest:
            raise ValueError('Packaged mod.json differs from the source manifest')
        required = ['LICENSE-EasyOffset', 'THIRD-PARTY-NOTICES.md']
        for field in ['modFiles', 'lateModFiles', 'libraryFiles']:
            required.extend(manifest.get(field, []))
        required.extend(copy['name'] for copy in manifest.get('fileCopies', []))
        if manifest.get('coverImage'):
            required.append(manifest['coverImage'])
        for name in required:
            if name not in names:
                raise ValueError(f'Manifest references a missing QMOD file: {name}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--qmod', type=Path, help='Also validate the packaged QMOD')
    parser.add_argument('--schema', type=Path, help='Use an already downloaded MBF schema')
    args = parser.parse_args()
    if args.schema:
        schema = json.loads(args.schema.read_text())
    else:
        with urllib.request.urlopen(SCHEMA_URL, timeout=30) as response:
            schema = json.load(response)
    schema = normalize_schema(schema)
    manifest = json.loads((ROOT / 'mod.json').read_text())
    Draft202012Validator.check_schema(schema)
    Draft202012Validator(schema).validate(manifest)
    version = validate_versions(manifest, os.environ.get('GITHUB_REF', ''))
    if args.qmod:
        validate_archive(args.qmod, manifest)
    output = os.environ.get('GITHUB_OUTPUT')
    if output:
        with open(output, 'a', encoding='utf-8') as handle:
            handle.write(f'version={version}\n')
    print(f'Version {version}: MBF manifest schema and version checks passed'
          + ('; QMOD contents checked' if args.qmod else ''))


if __name__ == '__main__':
    main()
