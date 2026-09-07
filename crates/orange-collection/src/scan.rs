//! Walk a music folder and list audio files the collection understands.
//! Tag reading is left to the caller (optional `tags` feature in orange-media).

use std::fs;
use std::path::{Path, PathBuf};
use std::time::SystemTime;

use orange_core::codecs::codec_for_extension;
use orange_core::paths::{path_to_file_url, url_file_stem};
use orange_core::song::{Song, UNKNOWN};

use crate::watcher::ScannedFile;

/// True when `path` has a supported audio extension.
pub fn is_audio_path(path: &Path) -> bool {
    path.extension()
        .and_then(|ext| ext.to_str())
        .and_then(codec_for_extension)
        .is_some()
}

/// Recursively scan `root` for audio files. Hidden entries and symlinks
/// are skipped so a scan cannot loop or pick up cache files.
pub fn scan_directory(root: &Path) -> Vec<ScannedFile> {
    let mut out = Vec::new();
    let mut stack = vec![root.to_path_buf()];
    let mut visited = 0usize;
    const MAX_ENTRIES: usize = 250_000;
    while let Some(dir) = stack.pop() {
        let Ok(entries) = fs::read_dir(&dir) else {
            continue;
        };
        for entry in entries.flatten() {
            visited += 1;
            if visited > MAX_ENTRIES {
                return out;
            }
            let path = entry.path();
            let name = entry.file_name();
            let name = name.to_string_lossy();
            if name.starts_with('.') {
                continue;
            }
            let Ok(meta) = entry.metadata() else {
                continue;
            };
            if meta.file_type().is_symlink() {
                continue;
            }
            if meta.is_dir() {
                stack.push(path);
                continue;
            }
            if !meta.is_file() || !is_audio_path(&path) {
                continue;
            }
            let mtime = meta
                .modified()
                .ok()
                .and_then(|t| t.duration_since(SystemTime::UNIX_EPOCH).ok())
                .map(|d| d.as_secs() as i64)
                .unwrap_or(0);
            out.push(ScannedFile {
                url: path_to_file_url(&path),
                mtime,
                size: meta.len() as i64,
            });
        }
    }
    out.sort_by(|a, b| a.url.cmp(&b.url));
    out
}

/// Build a [`Song`] from a filesystem path using folder layout as tags:
/// `Artist/Album/01 Title.flac` (relative to `root`).
pub fn song_from_path(path: &Path, root: &Path, directory_id: i64) -> Song {
    let stem = path
        .file_stem()
        .and_then(|s| s.to_str())
        .unwrap_or("")
        .to_string();
    let (track, title) = parse_track_title(&stem);
    let relative = path.strip_prefix(root).unwrap_or(path);
    let mut parents: Vec<String> = relative
        .parent()
        .unwrap_or_else(|| Path::new(""))
        .components()
        .filter_map(|c| c.as_os_str().to_str().map(str::to_string))
        .collect();
    let (artist, album) = match parents.len() {
        0 => (String::new(), String::new()),
        1 => (String::new(), parents.pop().unwrap_or_default()),
        _ => {
            let album = parents.pop().unwrap_or_default();
            let artist = parents.pop().unwrap_or_default();
            (artist, album)
        }
    };
    let meta = fs::metadata(path).ok();
    let mtime = meta
        .as_ref()
        .and_then(|m| m.modified().ok())
        .and_then(|t| t.duration_since(SystemTime::UNIX_EPOCH).ok())
        .map(|d| d.as_secs() as i64)
        .unwrap_or(UNKNOWN);
    let filesize = meta.map(|m| m.len() as i64).unwrap_or(UNKNOWN);
    Song {
        title,
        album: album.clone(),
        artist: artist.clone(),
        albumartist: artist,
        track,
        url: path_to_file_url(path),
        filesize,
        mtime,
        directory_id,
        ..Song::default()
    }
}

/// `01 - So What` / `01.So What` / `01 So What` → `(1, "So What")`.
pub fn parse_track_title(stem: &str) -> (i64, String) {
    let bytes = stem.as_bytes();
    let mut i = 0;
    while i < bytes.len() && bytes[i].is_ascii_digit() {
        i += 1;
    }
    if i == 0 || i == bytes.len() {
        return (UNKNOWN, stem.to_string());
    }
    let rest = stem[i..].trim_start_matches([' ', '-', '.', '_', ')']);
    if rest.is_empty() {
        return (UNKNOWN, stem.to_string());
    }
    let track = stem[..i].parse().unwrap_or(UNKNOWN);
    (track, rest.to_string())
}

/// Title shown in the playlist when the song has no tag.
pub fn fallback_title(url: &str) -> String {
    let stem = url_file_stem(url);
    parse_track_title(&stem).1
}

/// Convert a scanned file into a song, using `root` for Artist/Album layout.
pub fn song_from_scanned(file: &ScannedFile, root: &Path, directory_id: i64) -> Song {
    let path =
        orange_core::paths::file_url_to_path(&file.url).unwrap_or_else(|| PathBuf::from(&file.url));
    let mut song = song_from_path(&path, root, directory_id);
    song.url = file.url.clone();
    song.mtime = file.mtime;
    song.filesize = file.size;
    song
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn track_prefix_parsed() {
        assert_eq!(parse_track_title("01 - So What"), (1, "So What".into()));
        assert_eq!(parse_track_title("2.Freddie"), (2, "Freddie".into()));
        assert_eq!(
            parse_track_title("Blue in Green"),
            (UNKNOWN, "Blue in Green".into())
        );
    }

    #[test]
    fn scan_finds_audio_and_skips_hidden() {
        let root = std::env::temp_dir().join(format!(
            "orange-scan-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        let album = root.join("Miles Davis").join("Kind of Blue");
        fs::create_dir_all(&album).unwrap();
        fs::write(album.join("01 So What.flac"), b"").unwrap();
        fs::write(album.join("02 Freddie Freeloader.mp3"), b"").unwrap();
        fs::write(album.join("cover.jpg"), b"").unwrap();
        fs::write(album.join(".hidden.flac"), b"").unwrap();
        let scanned = scan_directory(&root);
        assert_eq!(scanned.len(), 2);
        let so_what = scanned
            .iter()
            .find(|f| f.url.contains("So%20What"))
            .expect("scanned So What");
        let song = song_from_scanned(so_what, &root, 7);
        assert_eq!(song.artist, "Miles Davis");
        assert_eq!(song.album, "Kind of Blue");
        assert_eq!(song.title, "So What");
        assert_eq!(song.track, 1);
        assert_eq!(song.directory_id, 7);
        let _ = fs::remove_dir_all(&root);
    }

    #[test]
    fn extension_gate() {
        assert!(is_audio_path(Path::new("a.flac")));
        assert!(is_audio_path(Path::new("a.MP3")));
        assert!(!is_audio_path(Path::new("a.jpg")));
        assert!(!is_audio_path(Path::new("a")));
    }
}
