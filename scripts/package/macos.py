#!/usr/bin/env python3
"""Create a native .app and DMG; recursively relocate non-system dylibs.

Requires macOS, Homebrew GStreamer, Xcode command-line tools and a release
binary. Ad-hoc signing permits local testing without a developer certificate.
"""
import hashlib
import os
from pathlib import Path
import plistlib
import shutil
import subprocess
import tomllib


def run(*args):
    return subprocess.check_output(args, text=True).strip()


ROOT = Path(__file__).resolve().parents[2]
os.chdir(ROOT)
VERSION = tomllib.loads(Path("Cargo.toml").read_text())["workspace"]["package"]["version"]
ARCH = run("uname", "-m")
OUT = ROOT / "target/packages"
APP = OUT / "Orange.app"
if APP.exists():
    shutil.rmtree(APP)
CONTENTS = APP / "Contents"
FRAMEWORKS = CONTENTS / "Frameworks"
for directory in (CONTENTS / "MacOS", CONTENTS / "Resources", CONTENTS / "Helpers", FRAMEWORKS):
    directory.mkdir(parents=True, exist_ok=True)
shutil.copy2("target/release/orange", CONTENTS / "MacOS/orange")
prefix = Path(run("brew", "--prefix", "gstreamer"))
mapped = {}


def install_binary(source, destination):
    source = Path(source).resolve()
    if source in mapped:
        return mapped[source]
    if destination.exists() and destination.resolve() != source:
        if hashlib.sha256(destination.read_bytes()).digest() != hashlib.sha256(source.read_bytes()).digest():
            raise RuntimeError(f"Conflicting bundled library: {destination.name}")
    destination.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source, destination)
    destination.chmod(destination.stat().st_mode | 0o200)
    mapped[source] = destination
    relocate(destination)
    if destination.suffix == ".dylib":
        relative = destination.relative_to(FRAMEWORKS)
        subprocess.check_call(["install_name_tool", "-id", f"@rpath/{relative}", str(destination)])
    return destination


def relocate(binary):
    for line in run("otool", "-L", str(binary)).splitlines()[1:]:
        dependency = line.strip().split(" (", 1)[0]
        if dependency.startswith(("/usr/lib/", "/System/Library/", "@")):
            continue
        source = Path(dependency)
        if not source.is_file():
            raise RuntimeError(f"Unresolved dependency: {dependency}")
        target = install_binary(source, FRAMEWORKS / source.name)
        relative = target.relative_to(FRAMEWORKS)
        subprocess.check_call(["install_name_tool", "-change", dependency, f"@rpath/{relative}", str(binary)])


for plugin in (prefix / "lib/gstreamer-1.0").glob("*.dylib"):
    install_binary(plugin, FRAMEWORKS / "gstreamer-1.0" / plugin.name)
scanner = prefix / "libexec/gstreamer-1.0/gst-plugin-scanner"
install_binary(scanner, CONTENTS / "Helpers/gst-plugin-scanner")
relocate(CONTENTS / "MacOS/orange")
for executable in (CONTENTS / "MacOS/orange", CONTENTS / "Helpers/gst-plugin-scanner"):
    subprocess.check_call(["install_name_tool", "-add_rpath", "@executable_path/../Frameworks", str(executable)])
# Modules loaded by GIO are outside the ordinary dylib dependency graph.
gio_prefix = Path(run("brew", "--prefix", "glib-networking")) if subprocess.call(["brew", "--prefix", "glib-networking"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL) == 0 else None
if gio_prefix:
    for module in (gio_prefix / "lib/gio/modules").glob("*.so"):
        install_binary(module, FRAMEWORKS / "gio/modules" / module.name)
iconset = OUT / "Orange.iconset"
iconset.mkdir(exist_ok=True)
for size in (16, 32, 128, 256, 512):
    for scale in (1, 2):
        suffix = "@2x" if scale == 2 else ""
        run("sips", "-z", str(size * scale), str(size * scale), "data/icons/128x128/com.goshapps.Orange.png", "--out", str(iconset / f"icon_{size}x{size}{suffix}.png"))
run("iconutil", "-c", "icns", str(iconset), "-o", str(CONTENTS / "Resources/Orange.icns"))
with (CONTENTS / "Info.plist").open("wb") as file:
    plistlib.dump({
        "CFBundleIdentifier": "com.goshapps.Orange", "CFBundleName": "Orange",
        "CFBundleDisplayName": "Orange Music Player", "CFBundleExecutable": "orange",
        "CFBundlePackageType": "APPL", "CFBundleIconFile": "Orange.icns",
        "CFBundleShortVersionString": VERSION.split("-")[0], "CFBundleVersion": VERSION.split("-")[0],
        "OrangeReleaseVersion": VERSION, "NSHighResolutionCapable": True,
        "LSMinimumSystemVersion": "13.0", "NSHumanReadableCopyright": "Made by Gosh · GPL-3.0-or-later",
        "CFBundleDocumentTypes": [{"CFBundleTypeName": "Audio and playlists", "CFBundleTypeRole": "Viewer", "CFBundleTypeExtensions": ["flac", "mp3", "wav", "ogg", "m3u", "m3u8", "pls", "xspf"]}],
    }, file)
for name in ("COPYING", "README.md", "PLATFORM_SUPPORT.md"):
    shutil.copy2(name, CONTENTS / "Resources" / name)
for binary in reversed(list(mapped.values())):
    run("codesign", "--force", "--sign", "-", str(binary))
run("codesign", "--force", "--deep", "--sign", "-", str(APP))
run("codesign", "--verify", "--deep", "--strict", str(APP))
run(str(CONTENTS / "MacOS/orange"), "--version")
staging = OUT / f"orange-{VERSION}-macos-{ARCH}"
if staging.exists():
    shutil.rmtree(staging)
staging.mkdir()
shutil.copytree(APP, staging / "Orange.app", dirs_exist_ok=True)
(staging / "Applications").symlink_to("/Applications", target_is_directory=True)
dmg = OUT / f"orange-{VERSION}-macos-{ARCH}.dmg"
run("hdiutil", "create", "-volname", "Orange", "-srcfolder", str(staging), "-ov", "-format", "UDZO", str(dmg))
archive = OUT / f"orange-{VERSION}-macos-{ARCH}.zip"
run("ditto", "-c", "-k", "--keepParent", str(APP), str(archive))
for artifact in (dmg, archive):
    artifact.with_suffix(artifact.suffix + ".sha256").write_text(f"{hashlib.sha256(artifact.read_bytes()).hexdigest()}  {artifact.name}\n")
