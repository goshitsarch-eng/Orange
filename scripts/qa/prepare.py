#!/usr/bin/env python3
"""Create a disposable complete collection and local audio for WebView QA."""
from pathlib import Path
import sqlite3
import sys
import wave

root = Path(sys.argv[1]).resolve()
repo = Path(__file__).resolve().parents[2]
music = root / "Música 日本/Artist/Album"
music.mkdir(parents=True, exist_ok=True)
db = root / "data/orange/orange/orange.db"
db.parent.mkdir(parents=True, exist_ok=True)
if db.exists():
    raise SystemExit("Refusing to replace an existing QA database; choose a fresh directory")
with sqlite3.connect(db) as conn:
    conn.executescript((repo / "data/schema/schema.sql").read_text())
    conn.execute("INSERT INTO directories(path,subdirs) VALUES (?,1)", (str(root / "Música 日本"),))
    directory = conn.execute("SELECT ROWID FROM directories").fetchone()[0]
    for number in (1, 2):
        path = music / f"{number:02} Track {number}.wav"
        with wave.open(str(path), "wb") as wav:
            wav.setnchannels(1)
            wav.setsampwidth(2)
            wav.setframerate(8000)
            wav.writeframes(b"\0\0" * 8000 * 3)
        conn.execute("INSERT INTO songs(title,artist,album,albumartist,track,url,length,directory_id,mtime,filesize) VALUES (?,?,?,?,?,?,?,?,?,?)", (f"Track {number}", "Artist", "Album", "Artist", number, path.as_uri(), 3_000_000_000, directory, int(path.stat().st_mtime), path.stat().st_size))
print(root)
