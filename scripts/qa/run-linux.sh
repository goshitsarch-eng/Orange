#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
qa_root=$(mktemp -d)
trap 'rm -rf "$qa_root"' EXIT
python3 scripts/qa/prepare.py "$qa_root"
export ORANGE_PROFILE_DIR="$qa_root"
export XDG_DATA_HOME="$qa_root/data" XDG_CONFIG_HOME="$qa_root/config" XDG_CACHE_HOME="$qa_root/cache"
export ORANGE_UI_QA_SCRIPT="$PWD/scripts/qa/native-webview.js" ORANGE_UI_QA_RESULT="$qa_root/result.json" ORANGE_UI_QA_EXIT=1 ORANGE_AUDIO_OUTPUT=null
cargo build -p orange-app --locked --features ui-qa
xvfb-run -a -s '-screen 0 1280x800x24' dbus-run-session -- timeout 120 target/debug/orange
python3 - "$qa_root/result.json" <<'PY'
import json,sys
result=json.load(open(sys.argv[1]))
print(json.dumps(result,indent=2,ensure_ascii=False))
assert result['ok'],result
PY
