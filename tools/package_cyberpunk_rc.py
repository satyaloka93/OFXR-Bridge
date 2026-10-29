#!/usr/bin/env python3
"""Package the tested Windows RC binaries without replacing a local installation."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import zipfile

root = Path(__file__).resolve().parents[1]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--build', type=Path, required=True, help='CMake Release directory')
p.add_argument('--output', type=Path, required=True)
a = p.parse_args()
tag = 'cyberpunk-sbs-v557-rc1'
name = 'OFXR-Bridge-Cyberpunk-SBS-RC1'
stage = a.output / name
if stage.exists():
    raise SystemExit(f'Refusing to overwrite staging directory: {stage}')
stage.mkdir(parents=True)
def copy(src, dest):
    target = stage / dest
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, target)
def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()
expected = {
    'OFXRBridgeTray.exe': '746c2e3b3a558f85a19dcefd2eab51072203e584a8ca7030b2abc270ec975439',
    'XR_APILAYER_XRFrameBridge_diagnostic.dll': 'fb24317f16c1f56c7bf4227262fd1f1d6a122683664e978204bc163574332633',
}
for filename, digest in expected.items():
    if sha(a.build / filename) != digest:
        raise SystemExit(f'{filename} does not match the tested RC1 binary')
copy(a.build / 'OFXRBridgeTray.exe', 'OFXRBridgeTray.exe')
for filename in ['XR_APILAYER_XRFrameBridge_diagnostic.dll', 'OFXR_vulkan_queue_layer.dll']:
    copy(a.build / filename, 'ofxr/' + filename)
copy(a.build / 'ofxr_bridge.ini', 'ofxr/ofxr_bridge.ini')
for filename in ['README.md', 'LICENSE', 'THIRD_PARTY.md']:
    copy(root / filename, filename)
for directory in ['licenses', 'external/OpenXR-SDK/LICENSES']:
    for src in (root / directory).rglob('*'):
        if src.is_file(): copy(src, src.relative_to(root))
copy(root / 'external/Vulkan-Headers/LICENSE.md', 'external/Vulkan-Headers/LICENSE.md')
# Include upstream linked documentation, but not machine-specific local notes.
for src in (root / 'docs').rglob('*.md'):
    if src.name != 'LOCAL_PANEL_FRAMEGEN.md': copy(src, src.relative_to(root))
copy(root / f'docs/releases/{tag}.md', 'START-HERE.md')
copy(root / f'docs/releases/{tag}.md', 'RELEASE_NOTES.md')
for mode in ['Apply-Cyberpunk-Test-Profile', 'Restore-Previous-Profile']:
    copy(root / f'scripts/{mode}.cmd', f'{mode}.cmd')
copy(root / 'scripts/cyberpunk-test-profile.ps1', 'scripts/cyberpunk-test-profile.ps1')
commit = subprocess.check_output(['git', '-C', str(root), 'rev-parse', 'HEAD'], text=True).strip()
(stage / 'BUILD_INFO.json').write_text(json.dumps(dict(tag=tag, source_commit=commit,
    source_url=f'https://github.com/satyaloka93/OFXR-Bridge/tree/{tag}',
    upstream='djules75/OFXR-Bridge', upstream_commit='f64d4b0aa7e15747bc2060c8f962b830a53b02ab',
    tested_binary_sha256=expected), indent=2) + '\n')
files = sorted(x for x in stage.rglob('*') if x.is_file())
(stage / 'SHA256SUMS.txt').write_text(''.join(f'{sha(x)}  {x.relative_to(stage).as_posix()}\n' for x in files))
archive = a.output / (name + '.zip')
with zipfile.ZipFile(archive, 'x', zipfile.ZIP_DEFLATED) as z:
    for x in sorted(stage.rglob('*')):
        if x.is_file(): z.write(x, x.relative_to(a.output))
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
(a.output / 'SHA256SUMS.txt').write_text(f'{sha(archive)}  {archive.name}\n')
print(json.dumps(dict(zip=str(archive), sha256=sha(archive), source_commit=commit,
    files=len(files)+1), indent=2))
