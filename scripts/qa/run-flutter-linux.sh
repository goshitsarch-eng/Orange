#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../../desktop"
qa_root=$(mktemp -d)
trap 'rm -rf "$qa_root"' EXIT
python3 ../scripts/qa/prepare.py "$qa_root"
export ORANGE_PROFILE_DIR="$qa_root" ORANGE_AUDIO_OUTPUT=null
# Flutter uses the platform GTK embedding; no WebKit helper namespace is needed.
xvfb-run -a -s '-screen 0 1400x1000x24' dbus-run-session -- \
  flutter test integration_test/desktop_test.dart -d linux
