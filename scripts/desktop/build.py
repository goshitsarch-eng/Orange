#!/usr/bin/env python3
"""One native build entry point, using the platform Flutter runner's Cargo hook."""
import argparse
from pathlib import Path
import platform
import subprocess
import shutil
import tomllib

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument('--debug', action='store_true')
parser.add_argument('--package', action='store_true')
args = parser.parse_args()
target = {'Linux': 'linux', 'Windows': 'windows', 'Darwin': 'macos'}[platform.system()]
mode = '--debug' if args.debug else '--release'
cargo_version = tomllib.loads((root / 'Cargo.toml').read_text())['workspace']['package']['version']
app_version = next(line.split(':', 1)[1].strip().split('+')[0] for line in (root / 'desktop/pubspec.yaml').read_text().splitlines() if line.startswith('version:'))
if cargo_version != app_version:
    raise SystemExit('Cargo and Flutter release versions disagree')
subprocess.check_call([shutil.which('flutter') or 'flutter', 'pub', 'get', '--enforce-lockfile'], cwd=root / 'desktop')
subprocess.check_call([shutil.which('flutter') or 'flutter', 'build', target, mode], cwd=root / 'desktop')
if args.package:
    if args.debug:
        raise SystemExit('Only release bundles can be packaged')
    if target in ('linux', 'macos'):
        subprocess.check_call(['python3', str(root / f'scripts/package/{target}.py')], cwd=root)
    else:
        import os
        gst = os.environ.get('GSTREAMER_1_0_ROOT_MSVC_X86_64')
        if not gst:
            raise SystemExit('Set GSTREAMER_1_0_ROOT_MSVC_X86_64 to the verified GStreamer SDK')
        subprocess.check_call(['pwsh', '-File', str(root / 'scripts/package/windows.ps1'), '-GStreamerRoot', gst], cwd=root)
