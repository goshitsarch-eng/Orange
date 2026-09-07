//! Files sidebar: browse the local filesystem the way Strawberry's Files tab does.

use std::fs;
use std::path::{Path, PathBuf};

use orange_collection::scan::is_audio_path;
use orange_core::paths::music_dir;
use orange_core::song::Song;

/// One directory listing row.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct FsEntry {
    pub name: String,
    pub path: PathBuf,
    pub is_dir: bool,
    pub is_audio: bool,
}

/// Current folder in the Files tab.
#[derive(Debug, Clone)]
pub struct FileBrowser {
    pub cwd: PathBuf,
    pub entries: Vec<FsEntry>,
    pub error: Option<String>,
}

impl Default for FileBrowser {
    fn default() -> Self {
        Self::at(music_dir())
    }
}

impl FileBrowser {
    pub fn at(path: PathBuf) -> Self {
        let mut browser = Self {
            cwd: path,
            entries: Vec::new(),
            error: None,
        };
        browser.refresh();
        browser
    }

    pub fn home() -> Self {
        let home = std::env::var("HOME").unwrap_or_else(|_| String::from("."));
        Self::at(PathBuf::from(home))
    }

    pub fn refresh(&mut self) {
        self.error = None;
        match list_entries(&self.cwd) {
            Ok(entries) => self.entries = entries,
            Err(e) => {
                self.entries.clear();
                self.error = Some(e);
            }
        }
    }

    pub fn enter(&mut self, path: PathBuf) {
        if path.is_dir() {
            self.cwd = path;
            self.refresh();
        }
    }

    pub fn go_up(&mut self) {
        if let Some(parent) = self.cwd.parent() {
            self.cwd = parent.to_path_buf();
            self.refresh();
        }
    }

    pub fn audio_in_cwd(&self) -> Vec<Song> {
        self.entries
            .iter()
            .filter(|e| e.is_audio)
            .map(|e| orange_collection::scan::song_from_path(&e.path, &self.cwd, -1))
            .collect()
    }
}

fn list_entries(dir: &Path) -> Result<Vec<FsEntry>, String> {
    let mut entries = Vec::new();
    let read = fs::read_dir(dir).map_err(|e| e.to_string())?;
    for entry in read.flatten() {
        let name = entry.file_name();
        let name = name.to_string_lossy();
        if name.starts_with('.') {
            continue;
        }
        let path = entry.path();
        let Ok(meta) = entry.metadata() else {
            continue;
        };
        if meta.file_type().is_symlink() {
            continue;
        }
        let is_dir = meta.is_dir();
        let is_audio = !is_dir && is_audio_path(&path);
        if !is_dir && !is_audio {
            continue;
        }
        entries.push(FsEntry {
            name: name.to_string(),
            path,
            is_dir,
            is_audio,
        });
    }
    entries.sort_by(|a, b| {
        b.is_dir.cmp(&a.is_dir).then(
            a.name
                .to_ascii_lowercase()
                .cmp(&b.name.to_ascii_lowercase()),
        )
    });
    Ok(entries)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn lists_audio_and_folders_skips_other() {
        let dir = std::env::temp_dir().join(format!(
            "orange-files-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        fs::create_dir_all(dir.join("Albums")).unwrap();
        fs::write(dir.join("track.flac"), b"").unwrap();
        fs::write(dir.join("notes.txt"), b"").unwrap();
        let browser = FileBrowser::at(dir.clone());
        assert!(browser
            .entries
            .iter()
            .any(|e| e.is_dir && e.name == "Albums"));
        assert!(browser
            .entries
            .iter()
            .any(|e| e.is_audio && e.name == "track.flac"));
        assert!(!browser.entries.iter().any(|e| e.name == "notes.txt"));
        let _ = fs::remove_dir_all(&dir);
    }
}
