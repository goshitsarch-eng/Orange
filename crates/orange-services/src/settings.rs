//! Versioned, atomic per-user settings. Legacy inputs are read-only.
use serde::{Deserialize, Serialize};
use std::io::{Read, Write};
use std::path::{Path, PathBuf};

#[derive(Debug, Clone, Default, Serialize, Deserialize, PartialEq)]
pub struct Station {
    pub name: String,
    pub url: String,
}

#[derive(Debug, Clone, Default, Serialize, Deserialize, PartialEq)]
pub struct Track {
    pub url: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub genre: String,
    pub length_ns: i64,
    pub year: i64,
    pub track: i64,
}
impl From<&orange_media::playback::QueuedTrack> for Track {
    fn from(t: &orange_media::playback::QueuedTrack) -> Self {
        Self {
            url: t.url.clone(),
            title: t.title.clone(),
            artist: t.artist.clone(),
            album: t.album.clone(),
            genre: t.genre.clone(),
            length_ns: t.length_ns,
            year: t.year,
            track: t.track,
        }
    }
}
impl From<Track> for orange_media::playback::QueuedTrack {
    fn from(t: Track) -> Self {
        Self {
            url: t.url,
            title: t.title,
            artist: t.artist,
            album: t.album,
            genre: t.genre,
            length_ns: t.length_ns,
            year: t.year,
            track: t.track,
        }
    }
}

#[derive(Debug, Clone, Serialize, Deserialize, PartialEq)]
#[serde(default)]
pub struct Settings {
    pub schema: u32,
    pub theme: String,
    pub volume: u8,
    pub equalizer: [f64; 10],
    pub stations: Vec<Station>,
    pub queue: std::sync::Arc<Vec<Track>>,
    pub cursor: Option<usize>,
    pub repeat: u8,
    pub shuffle: u8,
    pub window_width: u32,
    pub window_height: u32,
}
impl Default for Settings {
    fn default() -> Self {
        Self {
            schema: 1,
            theme: "system".into(),
            volume: 100,
            equalizer: [0.0; 10],
            stations: vec![],
            queue: std::sync::Arc::new(vec![]),
            cursor: None,
            repeat: 0,
            shuffle: 0,
            window_width: 1100,
            window_height: 740,
        }
    }
}
impl Settings {
    pub fn path() -> PathBuf {
        orange_core::paths::config_home()
            .join("orange")
            .join("desktop.json")
    }
    pub fn load(path: &Path) -> Result<Self, String> {
        let bytes = std::fs::File::open(path).and_then(|file| {
            let mut bytes = Vec::new();
            file.take(8 * 1024 * 1024 + 1).read_to_end(&mut bytes)?;
            Ok(bytes)
        });
        match bytes {
            Ok(bytes) => {
                if bytes.len() > 8 * 1024 * 1024 {
                    return Err("Settings file is too large; it has been preserved.".into());
                }
                let mut s: Self = serde_json::from_slice(&bytes)
                    .map_err(|e| format!("Cannot read settings; original preserved: {e}"))?;
                if s.schema != 1 {
                    return Err("Unsupported settings version; original preserved.".into());
                }
                s.volume = s.volume.min(100);
                s.repeat = s.repeat.min(3);
                s.shuffle = s.shuffle.min(2);
                s.theme = orange_core::appearance::AppearanceMode::parse(&s.theme)
                    .as_str()
                    .into();
                for gain in &mut s.equalizer {
                    *gain = if gain.is_finite() {
                        gain.clamp(-12.0, 12.0)
                    } else {
                        0.0
                    };
                }
                s.window_width = s.window_width.clamp(600, 7680);
                s.window_height = s.window_height.clamp(400, 4320);
                Ok(s)
            }
            Err(e) if e.kind() == std::io::ErrorKind::NotFound => {
                let legacy = path.parent().unwrap_or(Path::new(".")).join("orange.conf");
                Ok(std::fs::read_to_string(legacy)
                    .map(|text| Self::from_legacy(&text))
                    .unwrap_or_default())
            }
            Err(e) => Err(format!("Cannot load settings: {e}")),
        }
    }
    pub fn from_legacy(text: &str) -> Self {
        let mut s = Self::default();
        let mut section = "";
        for line in text.lines().map(str::trim) {
            if let Some(name) = line.strip_prefix('[').and_then(|v| v.strip_suffix(']')) {
                section = name;
                continue;
            }
            if let Some((key, value)) = line.split_once('=') {
                match (section, key.trim()) {
                    ("Appearance", "color_scheme") => {
                        s.theme = match value.trim() {
                            "1" => "light",
                            "2" => "dark",
                            _ => "system",
                        }
                        .into()
                    }
                    ("Player", "volume") => {
                        if let Ok(v) = value.trim().parse::<u8>() {
                            s.volume = v.min(100)
                        }
                    }
                    _ => {}
                }
            }
        }
        s
    }
    pub fn save(&self, path: &Path) -> Result<(), String> {
        let parent = path.parent().ok_or("Settings path has no parent")?;
        std::fs::create_dir_all(parent).map_err(|e| e.to_string())?;
        let mut file = tempfile::NamedTempFile::new_in(parent).map_err(|e| e.to_string())?;
        let data = serde_json::to_vec_pretty(self).map_err(|e| e.to_string())?;
        file.write_all(&data).map_err(|e| e.to_string())?;
        file.as_file().sync_all().map_err(|e| e.to_string())?;
        file.persist(path).map_err(|e| e.to_string())?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn atomic_settings_survive_replacement_and_unicode_paths() {
        let root = tempfile::tempdir().unwrap();
        let path = root.path().join("Música 日本").join("settings.json");
        let mut s = Settings {
            theme: "dark".into(),
            ..Settings::default()
        };
        s.save(&path).unwrap();
        s.volume = 42;
        s.stations.push(Station {
            name: "Jazz".into(),
            url: "https://example.org/jazz".into(),
        });
        s.save(&path).unwrap();
        assert_eq!(Settings::load(&path).unwrap(), s);
    }
    #[test]
    fn invalid_settings_are_rejected_and_preserved() {
        let root = tempfile::tempdir().unwrap();
        let path = root.path().join("settings.json");
        std::fs::write(&path, b"broken original").unwrap();
        assert!(Settings::load(&path).is_err());
        assert_eq!(std::fs::read(&path).unwrap(), b"broken original");
    }
    #[test]
    fn oversized_settings_are_bounded_and_preserved() {
        let root = tempfile::tempdir().unwrap();
        let path = root.path().join("settings.json");
        std::fs::File::create(&path)
            .unwrap()
            .set_len(16 * 1024 * 1024)
            .unwrap();
        assert!(Settings::load(&path).unwrap_err().contains("too large"));
        assert_eq!(std::fs::metadata(&path).unwrap().len(), 16 * 1024 * 1024);
    }
    #[test]
    fn imports_useful_qsettings_without_mutation() {
        let s =
            Settings::from_legacy("[Appearance]\r\ncolor_scheme=2\r\n[Player]\r\nvolume=38\r\n");
        assert_eq!(s.theme, "dark");
        assert_eq!(s.volume, 38);
    }
}
