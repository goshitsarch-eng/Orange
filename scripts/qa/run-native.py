#!/usr/bin/env python3
"""Exercise the native WebView in isolated profiles on Windows and macOS."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

repo = Path(__file__).resolve().parents[2]
os.chdir(repo)
subprocess.run(['cargo', 'build', '-p', 'orange-app', '--locked', '--features', 'ui-qa'], check=True)
binary = repo / 'target/debug' / ('orange.exe' if os.name == 'nt' else 'orange')


def run_case(root, script, arguments=()):
    subprocess.run([os.sys.executable, 'scripts/qa/prepare.py', str(root)], check=True)
    result = root / 'result.json'
    environment = os.environ | {
        'ORANGE_PROFILE_DIR': str(root),
        'ORANGE_UI_QA_SCRIPT': str(repo / 'scripts/qa' / script),
        'ORANGE_UI_QA_RESULT': str(result),
        'ORANGE_UI_QA_EXIT': '1',
        'ORANGE_AUDIO_OUTPUT': 'null',
    }
    process = subprocess.run([str(binary), *map(str, arguments)], env=environment, timeout=120)
    payload = json.loads(result.read_text(encoding='utf-8'))
    print(json.dumps(payload, indent=2, ensure_ascii=False))
    assert payload['ok'], payload
    process.check_returncode()


with tempfile.TemporaryDirectory(prefix='orange-qa-') as directory:
    root = Path(directory)
    run_case(root / 'main', 'native-webview.js')
    launch = root / 'launch'
    launch.mkdir()
    playlist = launch / 'launch.m3u8'
    playlist.write_text(
        '#EXTM3U\n#EXTINF:3,Launch 日本 First\n'
        'Música 日本/Artist/Album/01 Track 1.wav\n'
        'Música 日本/Artist/Album/02 Track 2.wav\n', encoding='utf-8')
    run_case(launch, 'launch-playlist.js', [playlist])
