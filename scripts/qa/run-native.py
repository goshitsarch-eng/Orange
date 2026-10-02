#!/usr/bin/env python3
"""Exercise the native WebView in an isolated profile on Windows and macOS."""
import json
import os
from pathlib import Path
import subprocess
import tempfile
repo=Path(__file__).resolve().parents[2]
os.chdir(repo)
subprocess.run(['cargo','build','-p','orange-app','--locked','--features','ui-qa'],check=True)
with tempfile.TemporaryDirectory(prefix='orange-qa-') as directory:
    root=Path(directory)
    subprocess.run([os.sys.executable,'scripts/qa/prepare.py',str(root)],check=True)
    result=root/'result.json'
    environment=os.environ | {'ORANGE_PROFILE_DIR':str(root),'ORANGE_UI_QA_SCRIPT':str(repo/'scripts/qa/native-webview.js'),'ORANGE_UI_QA_RESULT':str(result),'ORANGE_UI_QA_EXIT':'1','ORANGE_AUDIO_OUTPUT':'null'}
    binary=repo/'target/debug'/('orange.exe' if os.name=='nt' else 'orange')
    subprocess.run([str(binary)],env=environment,timeout=120,check=True)
    payload=json.loads(result.read_text())
    print(json.dumps(payload,indent=2,ensure_ascii=False))
    assert payload['ok'],payload
