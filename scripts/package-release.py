#!/usr/bin/env python3
"""Package an already built/tested clean checkout as a Windows x64 release."""
import argparse
import hashlib
from pathlib import Path
import re
import subprocess
import zipfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--version', required=True)
    args = parser.parse_args()
    if not re.fullmatch(r'[0-9][A-Za-z0-9.-]*', args.version):
        parser.error('version must contain only letters, digits, dots and hyphens')
    root = Path(__file__).resolve().parents[1]
    def git(*args):
        return subprocess.check_output(['git', '-C', str(root), *args], text=True).strip()
    if git('status', '--porcelain'):
        parser.error('commit or resolve working-tree changes before packaging')
    commit = git('rev-parse', 'HEAD')
    release = root / 'build' / 'Release'
    layer = 'XR_APILAYER_XRFrameBridge_diagnostic.dll'
    if (release / layer).read_bytes() != (release / 'ofxr' / layer).read_bytes():
        parser.error('tray layer copy differs from the main build DLL; rebuild first')
    files = {
        'OFXRBridgeTray.exe': release / 'OFXRBridgeTray.exe',
        'ofxr/' + layer: release / 'ofxr' / layer,
        'ofxr/ofxr_bridge.ini': release / 'ofxr' / 'ofxr_bridge.ini',
        'INSTALL.md': root / 'docs' / 'releases' / (args.version + '.md'),
        'LICENSE': root / 'LICENSE',
    }
    for folder, pattern in [('licenses', '*.txt'), ('docs', '*.md'), ('.okf', '*.md')]:
        for path in sorted((root / folder).glob(pattern)):
            files[path.relative_to(root).as_posix()] = path
    for name in ['Set-OFXRScheduling.ps1', 'analyze-scheduling.py', 'summarize-flight-log.ps1']:
        files['scripts/' + name] = root / 'scripts' / name
    for name in ['Apache-2.0.txt', 'MIT.txt']:
        files['licenses/OpenXR-' + name] = root / 'external/OpenXR-SDK/LICENSES' / name
    payload = {name: path.read_bytes() for name, path in files.items()}
    payload['INSTALL.md'] = payload['INSTALL.md'].replace(b'(../FORK_STATUS.md)', b'(docs/FORK_STATUS.md)')
    payload['BUILD-INFO.txt'] = (
        f'OFXR Bridge fork v{args.version}\nWindows x64 Release, VS2022\n'
        f'Source commit: {commit}\nInternal runtime identity: V116\n'
        f'Source: https://github.com/satyaloka93/OFXR-Bridge/tree/{commit}\n'
        'Build instructions: docs/BUILDING.md\n'
        'Package creation does not independently verify source/binary correspondence.\n'
    ).encode()
    digest = lambda data: hashlib.sha256(data).hexdigest()
    payload['PACKAGE-SHA256SUMS.txt'] = ''.join(
        f'{digest(data)}  {name}\n' for name, data in sorted(payload.items())
    ).encode()
    output = root / 'build/packages' / ('v' + args.version)
    output.mkdir(parents=True, exist_ok=True)
    archive = output / f'OFXR-Bridge-v{args.version}-windows-x64.zip'
    with zipfile.ZipFile(archive, 'w', zipfile.ZIP_DEFLATED) as z:
        for name, data in sorted(payload.items()):
            z.writestr(name, data)
    with zipfile.ZipFile(archive) as z:
        if z.testzip() is not None:
            raise RuntimeError('ZIP integrity check failed')
        for name, data in payload.items():
            if z.read(name) != data:
                raise RuntimeError('ZIP payload mismatch: ' + name)
    checksum = output / 'SHA256SUMS.txt'
    checksum.write_text(f'{digest(archive.read_bytes())}  {archive.name}\n')
    print(archive)
    print(checksum.read_text(), end='')


if __name__ == '__main__':
    main()
