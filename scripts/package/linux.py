#!/usr/bin/env python3
"""Package the Flutter bundle and its GStreamer/GTK dependency closure.

Build on the oldest supported distribution. glibc and graphics drivers remain
host dependencies; the build's actual glibc requirement is recorded in README.
"""
import hashlib
import os
from pathlib import Path
import platform
import re
import shutil
import subprocess
import tomllib

ROOT = Path(__file__).resolve().parents[2]
VERSION = tomllib.loads((ROOT / 'Cargo.toml').read_text())['workspace']['package']['version']
ARCH = platform.machine()
BUNDLE = ROOT / f'desktop/build/linux/{"arm64" if ARCH == "aarch64" else "x64"}/release/bundle'
OUT = ROOT / 'target/packages'
STAGE = OUT / f'orange-{VERSION}-linux-{ARCH}'
if not (BUNDLE / 'lib/liborange_bridge.so').is_file():
    raise SystemExit('Build the Flutter Linux release before packaging')
if STAGE.exists():
    shutil.rmtree(STAGE)
shutil.copytree(BUNDLE, STAGE, symlinks=False)
LIB = STAGE / 'lib'
# Baseline OS C library and graphics drivers must come from the host.
HOST = re.compile(r'^(ld-linux.*|lib(c|m|pthread|dl|rt|resolv|nss_.*)\.so.*|lib(GLX_mesa|EGL_mesa|vulkan|drm|gbm).*\.so.*)$')
seen = set()


def dependencies(binary):
    binary = Path(binary)
    if binary.resolve() in seen:
        return
    seen.add(binary.resolve())
    # Resolve the staged bundle first; Flutter's ephemeral directory may now
    # contain a different engine after a subsequent debug/UI test build.
    inspection_env = dict(os.environ)
    inspection_env['LD_LIBRARY_PATH'] = str(LIB) + os.pathsep + os.environ.get('LD_LIBRARY_PATH', '')
    result = subprocess.run(['ldd', str(binary)], capture_output=True, text=True, env=inspection_env)
    if result.returncode:
        raise RuntimeError(f'Cannot inspect dependency closure: {binary}')
    for line in result.stdout.splitlines():
        if '=> not found' in line:
            raise RuntimeError(f'Missing package dependency: {line.strip()}')
        match = re.match(r'\s*(\S+)\s+=>\s+(/\S+)', line)
        if not match or HOST.match(match[1]):
            continue
        destination = LIB / match[1]
        source = Path(match[2])
        if destination.exists() and hashlib.sha256(destination.read_bytes()).digest() != hashlib.sha256(source.read_bytes()).digest():
            raise RuntimeError(f'Conflicting bundled library: {destination.name}')
        if not destination.exists():
            shutil.copy2(source, destination)
        dependencies(destination)


prefix = Path(subprocess.check_output(['pkg-config', '--variable=prefix', 'gstreamer-1.0'], text=True).strip())
plugin_dir = os.environ.get('GST_PLUGIN_PATH') or subprocess.check_output(['pkg-config', '--variable=pluginsdir', 'gstreamer-1.0'], text=True).strip()
plugins = Path(plugin_dir.split(os.pathsep)[0])
if not plugins.is_dir():
    raise RuntimeError('GStreamer plugin directory was not found')
shutil.copytree(plugins, LIB / 'gstreamer-1.0', dirs_exist_ok=True, symlinks=False)
# Flutter opens GLES by name, which is invisible to ldd's static dependency graph.
gles = plugins.parent / 'libGLESv2.so.2'
if not gles.is_file():
    raise RuntimeError('Install libgles2 before producing a Flutter archive')
shutil.copy2(gles, LIB / gles.name)
scanner_value = os.environ.get('GST_PLUGIN_SCANNER') or subprocess.check_output(['pkg-config', '--variable=pluginscannerdir', 'gstreamer-1.0'], text=True).strip() + '/gst-plugin-scanner'
scanner = STAGE / 'libexec/gstreamer-1.0/gst-plugin-scanner'
scanner.parent.mkdir(parents=True, exist_ok=True)
shutil.copy2(scanner_value, scanner)
for gio in [prefix / 'lib/x86_64-linux-gnu/gio/modules', prefix / 'lib/aarch64-linux-gnu/gio/modules', prefix / 'lib/gio/modules']:
    if gio.is_dir():
        shutil.copytree(gio, LIB / 'gio/modules', dirs_exist_ok=True, symlinks=False)
for binary in list(LIB.rglob('*.so*')) + [STAGE / 'orange', STAGE / 'orange-cli', scanner]:
    if binary.is_file() and binary.read_bytes()[:4] == b'\x7fELF':
        dependencies(binary)
for name in ('orange', 'orange-cli'):
    (STAGE / name).rename(STAGE / f'{name}-bin')
    launcher = STAGE / name
    launcher.write_text('''#!/bin/sh
set -eu
bundle_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
export LD_LIBRARY_PATH="$bundle_dir/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export GST_PLUGIN_SYSTEM_PATH_1_0="$bundle_dir/lib/gstreamer-1.0"
export GST_PLUGIN_SCANNER_1_0="$bundle_dir/libexec/gstreamer-1.0/gst-plugin-scanner"
export GIO_MODULE_DIR="$bundle_dir/lib/gio/modules"
exec "$bundle_dir/''' + name + '''-bin" "$@"
''')
    launcher.chmod(0o755)
for source, directory in [('dist/unix/com.goshapps.Orange.desktop', 'share/applications'), ('dist/unix/com.goshapps.Orange.appdata.xml', 'share/metainfo')]:
    target = STAGE / directory
    target.mkdir(parents=True, exist_ok=True)
    shutil.copy2(ROOT / source, target)
for icon in (ROOT / 'data/icons').glob('*/com.goshapps.Orange.*'):
    target = STAGE / 'share/icons/hicolor' / icon.parent.name / 'apps'
    target.mkdir(parents=True, exist_ok=True)
    shutil.copy2(icon, target)
for name in ('COPYING', 'README.md', 'BUILDING.md', 'PLATFORM_SUPPORT.md', 'ARCHITECTURE.md', 'MIGRATION_AUDIT.md'):
    shutil.copy2(ROOT / name, STAGE)
shutil.copytree(ROOT / 'docs', STAGE / 'docs', dirs_exist_ok=True)
# Preserve installed dependency copyright/license notices.
licenses = STAGE / 'licenses'
licenses.mkdir()
roots = [prefix / 'share/doc', Path('/usr/share/doc')]
for docroot in roots:
    if docroot.is_dir():
        for copyright_file in docroot.glob('*/copyright'):
            destination = licenses / f'{copyright_file.parent.name}.txt'
            if not destination.exists():
                shutil.copy2(copyright_file, destination)
(STAGE / 'BUNDLE.txt').write_text(f'Orange {VERSION}\nBuild host: {platform.platform()}\nC library baseline: {platform.libc_ver()}\nHost graphics drivers are required.\nRun ./orange or ./orange-cli --help.\n')
archive = OUT / f'{STAGE.name}.tar.gz'
subprocess.check_call(['tar', '-C', str(OUT), '-czf', str(archive), STAGE.name])
archive.with_name(archive.name + '.sha256').write_text(f'{hashlib.sha256(archive.read_bytes()).hexdigest()}  {archive.name}\n')
print(archive)
