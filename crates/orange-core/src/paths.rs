//! Native user directories, standard file URLs and default music folder.

use std::path::{Path, PathBuf};

/// Native per-user data base; Linux preserves XDG locations.
pub fn data_home() -> PathBuf {
    if let Some(profile) = profile_dir() {
        return profile.join("data");
    }
    directories::BaseDirs::new()
        .map(|d| d.data_dir().to_owned())
        .unwrap_or_else(|| std::env::temp_dir().join("orange-no-home").join("data"))
}

pub fn cache_home() -> PathBuf {
    if let Some(profile) = profile_dir() {
        return profile.join("cache");
    }
    directories::BaseDirs::new()
        .map(|d| d.cache_dir().to_owned())
        .unwrap_or_else(|| std::env::temp_dir().join("orange-no-home").join("cache"))
}

pub fn config_home() -> PathBuf {
    if let Some(profile) = profile_dir() {
        return profile.join("config");
    }
    directories::BaseDirs::new()
        .map(|d| d.config_dir().to_owned())
        .unwrap_or_else(|| std::env::temp_dir().join("orange-no-home").join("config"))
}

/// Explicit isolated profile for QA and development, never an implicit
/// executable-relative configuration directory.
fn profile_dir() -> Option<PathBuf> {
    std::env::var_os("ORANGE_PROFILE_DIR")
        .map(PathBuf::from)
        .filter(|path| path.is_absolute())
}

pub fn home_dir() -> PathBuf {
    directories::BaseDirs::new()
        .map(|d| d.home_dir().to_owned())
        .unwrap_or_else(std::env::temp_dir)
}

pub fn music_dir() -> PathBuf {
    if let Some(dir) = std::env::var_os("XDG_MUSIC_DIR").filter(|d| !d.is_empty()) {
        return PathBuf::from(dir);
    }
    directories::UserDirs::new()
        .map(|d| d.audio_dir().unwrap_or(d.home_dir()).to_owned())
        .unwrap_or_else(std::env::temp_dir)
}

/// Standards-compliant file URL encoding, including Windows drive/UNC paths.
pub fn path_to_file_url(path: &Path) -> String {
    let absolute = if path.is_absolute() {
        path.to_owned()
    } else {
        std::env::current_dir()
            .unwrap_or_else(|_| std::env::temp_dir())
            .join(path)
    };
    url::Url::from_file_path(absolute)
        .map(|u| u.to_string())
        .unwrap_or_default()
}

pub fn file_url_to_path(value: &str) -> Option<PathBuf> {
    if value == "file://" {
        return None;
    }
    let parsed = url::Url::parse(value).ok()?;
    if parsed.scheme() != "file" || parsed.query().is_some() || parsed.fragment().is_some() {
        return None;
    }
    parsed.to_file_path().ok()
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
        let path = std::env::temp_dir().join("Música 日本/Kind of Blue/01 So What.flac");
        let url = path_to_file_url(&path);
        assert!(url.starts_with("file:///"));
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
