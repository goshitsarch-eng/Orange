#!/usr/bin/env python3
"""Validate runtime, native dependency gates, metadata and minimal permissions."""
import sys

REQUIRED_RUNTIME = "org.gnome.Platform"
# GNOME 49 is based on freedesktop 25.08 and supplies WebKit.
REQUIRED_VERSION = "49"
REQUIRED_APP_ID = "com.goshapps.Orange"
# SDK/runtime tokens that must never appear (Qt/KDE SDKs, arch-specific
# library paths). Plain words like "KDE icons" stay allowed: the canonical
# Orange KDE icons are required payload, not a runtime dependency.
FORBIDDEN = [
    "--filesystem=home",
    "--device=all",
    "--talk-name=org.mpris.MediaPlayer2.*",
    "libcosmic",
    "org.kde",
    "KF5",
    "KF6",
    "qt6",
    "Qt6",
    "KDSingleApplication",
    "lib64/x86_64",
    "/x86_64",
    "x86-64-linux",
    "aarch64-linux",
]
# The full plugin sets must be pinned by the build-time gst-inspect gate,
# and each set must be named so reviewers can see the coverage claim.
REQUIRED_GST = [
    "webkit2gtk-4.1",
    "gtk+-3.0",
    "gst-inspect-1.0",
    "base/good/bad/ugly",
    "libav",
    "lamemp3enc",
    "voaacenc",
    "flacenc",
    "vorbisenc",
    "opusenc",
    "wavpackenc",
]


def main() -> int:
    path = sys.argv[1]
    text = open(path, encoding="utf-8").read()
    failures = []

    def check(condition: bool, message: str) -> None:
        if not condition:
            failures.append(message)

    check(f"app-id: {REQUIRED_APP_ID}" in text, "missing app-id com.goshapps.Orange")
    check(REQUIRED_RUNTIME in text, "missing GNOME WebKit runtime")
    check(REQUIRED_VERSION in text, "missing runtime-version 49")
    check("rust" in text.lower(), "missing Rust toolchain module")
    check("com.goshapps.Orange.svg" in text, "missing canonical SVG icon install")
    check("com.goshapps.Orange.png" in text, "missing canonical PNG icon installs")
    check("com.goshapps.Orange.desktop" in text, "missing desktop file install")
    check("com.goshapps.Orange.appdata.xml" in text, "missing metainfo install")
    for plugin in REQUIRED_GST:
        check(plugin in text, f"missing GStreamer module {plugin}")
    for word in FORBIDDEN:
        check(word not in text, f"forbidden token present: {word}")
    check("org.mpris.MediaPlayer2" in text, "missing MPRIS D-Bus finish-arg")
    check("cargo-sources.json" in text, "missing pinned cargo sources include")
    check('CARGO_NET_OFFLINE: "true"' in text, "cargo build is not offline-hermetic")

    if failures:
        print(f"{path}: FAIL")
        for failure in failures:
            print(f"  - {failure}")
        return 1
    print(f"{path}: OK (multi-arch contract holds)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
