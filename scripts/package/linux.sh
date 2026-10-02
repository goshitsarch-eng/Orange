#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
version=$(python3 -c 'import tomllib; print(tomllib.load(open("Cargo.toml","rb"))["workspace"]["package"]["version"])')
arch=$(uname -m)
package="orange-${version}-linux-${arch}"
stage="target/packages/$package"
rm -rf "$stage"
mkdir -p "$stage/bin" "$stage/share/applications" "$stage/share/metainfo" "$stage/share/icons/hicolor" "target/packages"
install -m755 target/release/orange "$stage/bin/orange"
install -m644 dist/unix/com.goshapps.Orange.desktop "$stage/share/applications/"
install -m644 dist/unix/com.goshapps.Orange.appdata.xml "$stage/share/metainfo/"
for icon in data/icons/*/com.goshapps.Orange.*; do
  size=$(basename "$(dirname "$icon")")
  install -Dm644 "$icon" "$stage/share/icons/hicolor/$size/apps/$(basename "$icon")"
done
cp COPYING README.md PLATFORM_SUPPORT.md BUILDING.md MIGRATION_AUDIT.md ARCHITECTURE.md CONTRIBUTING.md CHANGELOG.md "$stage/"
cp -R docs "$stage/"
tar -C target/packages -czf "target/packages/$package.tar.gz" "$package"
cd target/packages
sha256sum "$package.tar.gz" > "$package.tar.gz.sha256"
