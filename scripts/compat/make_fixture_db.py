#!/usr/bin/env python3
"""Write a 2.1.5-shaped collection fixture DB with CPython's sqlite3.

Independent oracle for `just test-compat`: the writer shares no code with
the Rust reader (rusqlite), so a passing read proves SQL-level
read-compatibility, not self-agreement.

Usage: make_fixture_db.py OUTPUT.db
"""
import os
import sqlite3
import sys

SCHEMA = """
CREATE TABLE schema_version (version INTEGER NOT NULL);
INSERT INTO schema_version (version) VALUES (22);
CREATE TABLE directories (path TEXT NOT NULL, subdirs INTEGER NOT NULL);
INSERT INTO directories (path, subdirs) VALUES ('/music', 1);
CREATE TABLE songs (
  title TEXT, artist TEXT, album TEXT, year INTEGER NOT NULL DEFAULT -1,
  genre TEXT, track INTEGER NOT NULL DEFAULT -1, length INTEGER NOT NULL DEFAULT 0,
  url TEXT NOT NULL, playcount INTEGER NOT NULL DEFAULT 0,
  rating INTEGER DEFAULT -1
);
INSERT INTO songs (title, artist, album, year, genre, track, length, url, playcount, rating) VALUES
  ('Blue in Green', 'Miles Davis', 'Kind of Blue', 1959, 'Jazz', 3, 329000000000, 'file:///music/blue.flac', 42, 5),
  ('So What', 'Miles Davis', 'Kind of Blue', 1959, 'Jazz', 1, 545000000000, 'file:///music/sowhat.flac', 43, 5);
"""


def main() -> None:
    output = sys.argv[1]
    try:
        os.remove(output)
    except FileNotFoundError:
        pass
    conn = sqlite3.connect(output)
    try:
        conn.executescript(SCHEMA)
        conn.commit()
    finally:
        conn.close()
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
