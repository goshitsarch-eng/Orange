#!/usr/bin/env bash
set -euo pipefail
: "${ORANGE_VERSION:?Missing release version}"
: "${ORANGE_ARCH:?Missing release architecture}"
case "$ORANGE_ARCH" in x86_64|aarch64) ;; *) exit 1 ;; esac
test "$(uname -m)" = "$ORANGE_ARCH"
test "$ORANGE_VERSION" = "$(python3 dist/scripts/release-version.py)"
package="orange-$ORANGE_VERSION-linux-$ORANGE_ARCH"
mkdir -p staging release-artifacts
DESTDIR="$PWD/staging" cmake --install build --strip
mkdir -p "staging/$package"
cp -a staging/usr/local/. "staging/$package/"
cp COPYING "staging/$package/"
mkdir -p "staging/$package/licenses"
cp /tmp/kdsingleapplication/LICENSE.txt "staging/$package/licenses/KDSingleApplication.txt"
cp docs/releases.md "staging/$package/INSTALL.md"
# Include the exact runtime dependency resolution from the build machine.
ldd build/orange > "staging/$package/DEPENDENCIES.txt"
if grep -q 'not found' "staging/$package/DEPENDENCIES.txt"; then exit 1; fi
tar -C staging -czf "release-artifacts/$package.tar.gz" "$package"
tar -tzf "release-artifacts/$package.tar.gz" > staging/archive-files.txt
grep -q "$package/bin/orange$" staging/archive-files.txt
grep -q "$package/share/applications/com.goshapps.Orange.desktop$" staging/archive-files.txt
