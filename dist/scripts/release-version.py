#!/usr/bin/env python3
"""Read the authoritative Orange version and optionally format its release notes."""
import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def release_version():
    contents = (ROOT / "cmake/Version.cmake").read_text()
    parts = []
    for name in ("MAJOR", "MINOR", "PATCH"):
        match = re.search(rf"^set\(STRAWBERRY_VERSION_{name} ([0-9]+)\)$", contents, re.M)
        if not match:
            raise ValueError(f"Missing or invalid {name} version")
        parts.append(match.group(1))
    if re.search(r"^set\(STRAWBERRY_VERSION_PRERELEASE", contents, re.M):
        raise ValueError("Stable releases must not set STRAWBERRY_VERSION_PRERELEASE")
    return ".".join(parts)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--notes", action="store_true")
    args = parser.parse_args()
    version = release_version()
    if args.notes:
        changelog = (ROOT / "Changelog").read_text()
        match = re.search(rf"^Version {re.escape(version)} .*?\n(.*?)(?=^Version |\Z)", changelog, re.M | re.S)
        if not match:
            raise ValueError(f"No changelog entry for {version}")
        print(match.group(1).strip())
        print("\nDownloads: Linux install archives and Flatpak bundles for x86_64 (x64) and aarch64 (ARM64), plus source and SHA256SUMS.")
        print("\nLinux archives use Ubuntu 24.04 system libraries; use Flatpak for a managed cross-distribution runtime.")
        print("\nInstallation: https://github.com/goshitsarch-eng/Orange/blob/v" + version + "/docs/releases.md")
    else:
        print(version)
