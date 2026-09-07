//! Filesystem helpers: XDG locations, `file://` URLs, default music folder.

use std::path::{Path, PathBuf};

/// `$XDG_DATA_HOME` or `~/.local/share`.
pub fn data_home() -> String {
    std::env::var("XDG_DATA_HOME").unwrap_or_else(|_| {
        let home = std::env::var("HOME").unwrap_or_else(|_| String::from("~"));
        format!("{home}/.local/share")
    })
}

/// `$XDG_CACHE_HOME` or `~/.cache`.
pub fn cache_home() -> String {
    std::env::var("XDG_CACHE_HOME").unwrap_or_else(|_| {
        let home = std::env::var("HOME").unwrap_or_else(|_| String::from("~"));
        format!("{home}/.cache")
    })
}

/// Default music folder: `$XDG_MUSIC_DIR`, else `~/Music`.
pub fn music_dir() -> PathBuf {
    if let Ok(dir) = std::env::var("XDG_MUSIC_DIR") {
        let trimmed = dir.trim();
        if !trimmed.is_empty() {
            return PathBuf::from(trimmed);
        }
    }
    let home = std::env::var("HOME").unwrap_or_else(|_| String::from("."));
    PathBuf::from(home).join("Music")
}

/// Encode a local path as a `file://` URL (spaces and non-ASCII percent-encoded).
pub fn path_to_file_url(path: &Path) -> String {
    let absolute = if path.is_absolute() {
        path.to_path_buf()
    } else {
        std::env::current_dir()
            .unwrap_or_else(|_| PathBuf::from("."))
            .join(path)
    };
    let raw = absolute.to_string_lossy();
    let mut out = String::from("file://");
    for &byte in raw.as_bytes() {
        match byte {
            b'A'..=b'Z' | b'a'..=b'z' | b'0'..=b'9' | b'/' | b'-' | b'_' | b'.' | b'~' => {
                out.push(byte as char);
            }
            other => out.push_str(&format!("%{other:02X}")),
        }
    }
    out
}

/// Decode a `file://` URL back to a local path. Remote URLs return `None`.
pub fn file_url_to_path(url: &str) -> Option<PathBuf> {
    let rest = url.strip_prefix("file://")?;
    let decoded = percent_decode(rest)?;
    Some(PathBuf::from(decoded))
}

fn percent_decode(input: &str) -> Option<String> {
    let bytes = input.as_bytes();
    let mut out = Vec::with_capacity(bytes.len());
    let mut i = 0;
    while i < bytes.len() {
        match bytes[i] {
            b'%' if i + 2 < bytes.len() => {
                let hex = std::str::from_utf8(&bytes[i + 1..i + 3]).ok()?;
                out.push(u8::from_str_radix(hex, 16).ok()?);
                i += 3;
            }
            b => {
                out.push(b);
                i += 1;
            }
        }
    }
    String::from_utf8(out).ok()
}

/// Last path segment of a URL or filesystem path, for untitled tracks.
pub fn url_file_stem(url: &str) -> String {
    let trimmed = url.trim_end_matches('/');
    let name = trimmed.rsplit(['/', '\\']).next().unwrap_or(trimmed);
    let decoded = percent_decode(name).unwrap_or_else(|| name.to_string());
    decoded
        .rsplit_once('.')
        .map(|(stem, _)| stem.to_string())
        .unwrap_or(decoded)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn file_url_round_trip_encodes_spaces() {
        let path = Path::new("/home/u/Music/Kind of Blue/01 So What.flac");
        let url = path_to_file_url(path);
        assert!(url.starts_with("file:///home/u/Music/"));
        assert!(url.contains("Kind%20of%20Blue"));
        assert!(url.contains("01%20So%20What.flac"));
        assert_eq!(file_url_to_path(&url).unwrap(), path);
    }

    #[test]
    fn remote_urls_are_not_paths() {
        assert!(file_url_to_path("https://stream.example/x").is_none());
    }

    #[test]
    fn stem_strips_extension_and_encoding() {
        assert_eq!(
            url_file_stem("file:///music/01%20So%20What.flac"),
            "01 So What"
        );
        assert_eq!(url_file_stem("https://x/y.opus"), "y");
    }
}
