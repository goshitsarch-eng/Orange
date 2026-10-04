#!/usr/bin/env python3
"""Stage the pinned SDK and locked Dart packages for an offline Flatpak build.

Run on the same architecture as the Flatpak target. Package downloads happen
before the build sandbox; no network permission is granted to compilation.
"""
import json
import os
from pathlib import Path
import shutil
import subprocess

ROOT = Path(__file__).resolve().parents[2]
flutter = shutil.which('flutter')
if not flutter:
    raise SystemExit('Install the Flutter version in .fvmrc first')
sdk = Path(flutter).resolve().parent.parent
version = json.loads(subprocess.check_output(['flutter', '--version', '--machine'], text=True))['frameworkVersion']
expected = json.loads((ROOT / '.fvmrc').read_text())['flutter']
if version != expected:
    raise SystemExit(f'Use Flutter {expected}, found {version}')
subprocess.check_call(['flutter', 'precache', '--linux'])
subprocess.check_call(['flutter', 'pub', 'get', '--enforce-lockfile'], cwd=ROOT / 'desktop')
cache = Path(os.environ.get('PUB_CACHE', str(Path.home() / '.pub-cache')))
stage = ROOT / '.flatpak-input'
stage.mkdir(exist_ok=True)
for source, destination in [(sdk, stage / 'flutter'), (cache / 'hosted', stage / 'pub-cache/hosted'), (cache / 'hosted-hashes', stage / 'pub-cache/hosted-hashes')]:
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(source, destination, symlinks=False)
print(f'Offline Flutter {version} inputs staged in {stage}')
