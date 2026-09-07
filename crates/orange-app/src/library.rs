//! Collection shell state: folders, scan, tree, saved playlists.
//! Toolkit-independent so every `cargo test` covers library management.

use std::collections::HashSet;
use std::path::{Path, PathBuf};

use orange_collection::filter::CollectionFilter;
use orange_collection::scan::{scan_directory, song_from_scanned};
use orange_collection::tree::{self, ArtistNode, CollectionStats, GroupBy};
use orange_collection::watcher::{diff_scan, KnownFile};
use orange_core::identity;
use orange_core::paths::{self, music_dir};
use orange_core::song::Song;
use orange_db::library::{self, MusicDirectory, SavedPlaylist};
use orange_db::{open_collection, OpenMode};

/// Result of adding or rescanning a music folder.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct ScanReport {
    pub path: String,
    pub added: usize,
    pub changed: usize,
    pub removed: usize,
    pub unchanged: usize,
    pub songs: usize,
}

/// Live collection shown in the Rhythmbox-style browser.
#[derive(Debug, Clone)]
pub struct CollectionState {
    pub db_path: PathBuf,
    pub directories: Vec<MusicDirectory>,
    pub songs: Vec<Song>,
    pub playlists: Vec<SavedPlaylist>,
    pub search: String,
    pub group_by: GroupBy,
    pub expanded_artists: HashSet<String>,
    pub expanded_albums: HashSet<String>,
    pub add_path: String,
    pub last_error: Option<String>,
    pub last_scan: Option<ScanReport>,
    /// Rhythmbox-style Genre / Artist / Album selection.
    pub browser: LibraryBrowser,
}

impl Default for CollectionState {
    fn default() -> Self {
        Self {
            db_path: PathBuf::new(),
            directories: Vec::new(),
            songs: Vec::new(),
            playlists: Vec::new(),
            search: String::new(),
            group_by: GroupBy::default(),
            expanded_artists: HashSet::new(),
            expanded_albums: HashSet::new(),
            add_path: music_dir().display().to_string(),
            last_error: None,
            last_scan: None,
            browser: LibraryBrowser::default(),
        }
    }
}

impl CollectionState {
    /// Open (or create) the Orange collection database and load it.
    pub fn open() -> Self {
        let db_path = PathBuf::from(identity::collection_db_path(&paths::data_home()));
        let mut state = Self {
            db_path,
            add_path: music_dir().display().to_string(),
            ..Self::default()
        };
        state.reload();
        state
    }

    pub fn reload(&mut self) {
        self.last_error = None;
        match open_collection(&self.db_path, OpenMode::ReadWrite) {
            Ok(conn) => {
                self.directories = library::list_directories(&conn).unwrap_or_default();
                self.songs = library::load_songs(&conn).unwrap_or_default();
                self.playlists = library::list_playlists(&conn).unwrap_or_default();
            }
            Err(e) => {
                self.last_error = Some(e.to_string());
                self.directories.clear();
                self.songs.clear();
                self.playlists.clear();
            }
        }
    }

    pub fn filter(&self) -> CollectionFilter {
        CollectionFilter::parse(&self.search)
    }

    pub fn tree(&self) -> Vec<ArtistNode> {
        tree::build_tree(&self.songs, self.group_by, &self.filter())
    }

    pub fn stats(&self) -> CollectionStats {
        tree::stats(&self.tree())
    }

    pub fn album_key(artist: &str, album: &str) -> String {
        format!("{artist}\u{1f}{album}")
    }

    pub fn artist_expanded(&self, name: &str) -> bool {
        !self.search.is_empty() || self.expanded_artists.contains(name)
    }

    pub fn album_expanded(&self, artist: &str, album: &str) -> bool {
        !self.search.is_empty()
            || self
                .expanded_albums
                .contains(&Self::album_key(artist, album))
    }

    pub fn toggle_artist(&mut self, name: &str) {
        if !self.expanded_artists.remove(name) {
            self.expanded_artists.insert(name.to_string());
        }
    }

    pub fn toggle_album(&mut self, artist: &str, album: &str) {
        let key = Self::album_key(artist, album);
        if !self.expanded_albums.remove(&key) {
            self.expanded_albums.insert(key);
        }
    }

    /// Add `path` as a collection folder and scan it.
    pub fn add_folder(&mut self, path: &str) -> Result<ScanReport, String> {
        let trimmed = path.trim();
        if trimmed.is_empty() {
            return Err("Enter a folder path.".into());
        }
        let root = PathBuf::from(trimmed);
        if !root.is_dir() {
            return Err(format!("Not a folder: {trimmed}"));
        }
        let canonical = root.canonicalize().unwrap_or(root);
        let conn =
            open_collection(&self.db_path, OpenMode::ReadWrite).map_err(|e| e.to_string())?;
        let id = library::add_directory(&conn, &canonical.display().to_string())
            .map_err(|e| e.to_string())?;
        drop(conn);
        let report = self.scan_directory_id(id, &canonical)?;
        self.reload();
        self.last_scan = Some(report.clone());
        Ok(report)
    }

    pub fn remove_folder(&mut self, id: i64) -> Result<(), String> {
        let conn =
            open_collection(&self.db_path, OpenMode::ReadWrite).map_err(|e| e.to_string())?;
        library::remove_directory(&conn, id).map_err(|e| e.to_string())?;
        drop(conn);
        self.reload();
        Ok(())
    }

    /// Rescan every collection folder.
    pub fn rescan_all(&mut self) -> Result<ScanReport, String> {
        let dirs = self.directories.clone();
        if dirs.is_empty() {
            return Err("Add a music folder first.".into());
        }
        let mut total = ScanReport::default();
        for dir in dirs {
            let report = self.scan_directory_id(dir.id, Path::new(&dir.path))?;
            total.added += report.added;
            total.changed += report.changed;
            total.removed += report.removed;
            total.unchanged += report.unchanged;
            total.songs += report.songs;
            total.path = dir.path;
        }
        self.reload();
        self.last_scan = Some(total.clone());
        Ok(total)
    }

    fn scan_directory_id(&mut self, directory_id: i64, root: &Path) -> Result<ScanReport, String> {
        let scanned = scan_directory(root);
        let conn =
            open_collection(&self.db_path, OpenMode::ReadWrite).map_err(|e| e.to_string())?;
        let known = library::known_files(&conn, directory_id).unwrap_or_default();
        let known_files: Vec<KnownFile> = known
            .iter()
            .map(|s| KnownFile {
                url: s.url.clone(),
                mtime: s.mtime,
            })
            .collect();
        let diff = diff_scan(&scanned, &known_files);
        let songs: Vec<Song> = scanned
            .iter()
            .map(|file| {
                let mut song = song_from_scanned(file, root, directory_id);
                enrich_tags(&mut song);
                song
            })
            .collect();
        library::replace_directory_songs(&conn, directory_id, &songs).map_err(|e| e.to_string())?;
        Ok(ScanReport {
            path: root.display().to_string(),
            added: diff.added.len(),
            changed: diff.changed.len(),
            removed: diff.removed.len(),
            unchanged: diff.unchanged,
            songs: songs.len(),
        })
    }

    pub fn songs_for_artist(&self, name: &str) -> Vec<Song> {
        self.tree()
            .into_iter()
            .find(|a| a.name == name)
            .map(|a| tree::flatten_songs(&[a]))
            .unwrap_or_default()
    }

    pub fn songs_for_album(&self, artist: &str, album: &str) -> Vec<Song> {
        self.tree()
            .iter()
            .find(|a| a.name == artist)
            .and_then(|a| a.albums.iter().find(|al| al.name == album))
            .map(|al| al.songs.clone())
            .unwrap_or_default()
    }

    pub fn song_by_url(&self, url: &str) -> Option<&Song> {
        self.songs.iter().find(|s| s.url == url)
    }

    pub fn load_saved_playlist(&self, id: i64) -> Result<Vec<Song>, String> {
        let conn = open_collection(&self.db_path, OpenMode::ReadOnly).map_err(|e| e.to_string())?;
        library::load_playlist_songs(&conn, id).map_err(|e| e.to_string())
    }

    /// Tracks in the current Rhythmbox browser view (search + genre/artist/album).
    pub fn visible_tracks(&self) -> Vec<Song> {
        self.browser.tracks(&self.songs, &self.search)
    }
}

/// Rhythmbox library browser: Genre, then Artist, then Album, then tracks.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct LibraryBrowser {
    pub genre: Option<String>,
    pub artist: Option<String>,
    pub album: Option<String>,
}

impl LibraryBrowser {
    /// Selecting a genre clears artist and album, matching Rhythmbox.
    pub fn select_genre(&mut self, genre: Option<String>) {
        self.genre = genre.filter(|g| !g.is_empty());
        self.artist = None;
        self.album = None;
    }

    pub fn select_artist(&mut self, artist: Option<String>) {
        self.artist = artist.filter(|a| !a.is_empty());
        self.album = None;
    }

    pub fn select_album(&mut self, album: Option<String>) {
        self.album = album.filter(|a| !a.is_empty());
    }

    pub fn genres(&self, songs: &[Song], search: &str) -> Vec<(String, usize)> {
        counted_names(
            self.scoped(songs, search, false, false, false),
            display_genre,
        )
    }

    pub fn artists(&self, songs: &[Song], search: &str) -> Vec<(String, usize)> {
        counted_names(self.scoped(songs, search, true, false, false), |s| {
            s.display_artist().to_string()
        })
    }

    pub fn albums(&self, songs: &[Song], search: &str) -> Vec<(String, usize)> {
        counted_names(self.scoped(songs, search, true, true, false), |s| {
            s.display_album().to_string()
        })
    }

    pub fn tracks(&self, songs: &[Song], search: &str) -> Vec<Song> {
        let mut out: Vec<Song> = self
            .scoped(songs, search, true, true, true)
            .into_iter()
            .cloned()
            .collect();
        out.sort_by(|a, b| {
            a.display_album()
                .to_ascii_lowercase()
                .cmp(&b.display_album().to_ascii_lowercase())
                .then(a.disc.cmp(&b.disc))
                .then(a.track.cmp(&b.track))
                .then(
                    a.display_title()
                        .to_ascii_lowercase()
                        .cmp(&b.display_title().to_ascii_lowercase()),
                )
        });
        out
    }

    fn scoped<'a>(
        &self,
        songs: &'a [Song],
        search: &str,
        use_genre: bool,
        use_artist: bool,
        use_album: bool,
    ) -> Vec<&'a Song> {
        let filter = CollectionFilter::parse(search);
        songs
            .iter()
            .filter(|song| {
                if !filter.matches(song) {
                    return false;
                }
                if use_genre {
                    if let Some(genre) = &self.genre {
                        if display_genre(song) != *genre {
                            return false;
                        }
                    }
                }
                if use_artist {
                    if let Some(artist) = &self.artist {
                        if song.display_artist() != artist {
                            return false;
                        }
                    }
                }
                if use_album {
                    if let Some(album) = &self.album {
                        if song.display_album() != album {
                            return false;
                        }
                    }
                }
                true
            })
            .collect()
    }
}

fn display_genre(song: &Song) -> String {
    let genre = song.genre.trim();
    if genre.is_empty() {
        String::from("Unknown")
    } else {
        song.genre.clone()
    }
}

fn counted_names(songs: Vec<&Song>, name: impl Fn(&Song) -> String) -> Vec<(String, usize)> {
    let mut counts: Vec<(String, usize)> = Vec::new();
    for song in songs {
        let key = name(song);
        if let Some((_, count)) = counts.iter_mut().find(|(existing, _)| existing == &key) {
            *count += 1;
        } else {
            counts.push((key, 1));
        }
    }
    counts.sort_by_key(|a| a.0.to_ascii_lowercase());
    counts
}

/// Status-bar summary like Rhythmbox: `12 songs, 48:12`.
pub fn track_list_summary(songs: &[Song]) -> String {
    let secs: i64 = songs.iter().map(|s| s.length_secs().max(0)).sum();
    format!(
        "{} songs, {}",
        songs.len(),
        orange_core::song::format_duration_secs(secs)
    )
}

/// Built-in smart playlists that run against the loaded collection.
pub fn smart_never_played(songs: &[Song]) -> Vec<Song> {
    songs.iter().filter(|s| s.playcount <= 0).cloned().collect()
}

pub fn smart_highest_rated(songs: &[Song]) -> Vec<Song> {
    let mut out: Vec<Song> = songs.iter().filter(|s| s.rating > 0.0).cloned().collect();
    out.sort_by(|a, b| {
        b.rating
            .partial_cmp(&a.rating)
            .unwrap_or(std::cmp::Ordering::Equal)
    });
    out
}

pub fn smart_most_played(songs: &[Song]) -> Vec<Song> {
    let mut out = songs.to_vec();
    out.sort_by_key(|a| std::cmp::Reverse(a.playcount));
    out.truncate(50);
    out
}

fn enrich_tags(song: &mut Song) {
    #[cfg(feature = "tags")]
    {
        let Some(path) = paths::file_url_to_path(&song.url) else {
            return;
        };
        if let Ok(tags) = orange_media::tagger::read_tags(&path) {
            if !tags.title.is_empty() {
                song.title = tags.title;
            }
            if !tags.artist.is_empty() {
                song.artist = tags.artist.clone();
                if song.albumartist.is_empty() {
                    song.albumartist = tags.artist;
                }
            }
            if !tags.album.is_empty() {
                song.album = tags.album;
            }
            if !tags.genre.is_empty() {
                song.genre = tags.genre;
            }
            if let Some(year) = tags.year {
                song.year = year as i64;
            }
            if let Some(track) = tags.track {
                song.track = track as i64;
            }
            if let Some(disc) = tags.disc {
                song.disc = disc as i64;
            }
            if tags.duration_secs > 0 {
                song.length_ns = tags.duration_secs as i64 * 1_000_000_000;
            }
        }
    }
    #[cfg(not(feature = "tags"))]
    {
        let _ = song;
    }
}

#[cfg(test)]
mod tests {
    use super::*;
    use std::fs;

    fn unique_dir(name: &str) -> PathBuf {
        let dir = std::env::temp_dir().join(format!(
            "orange-lib-{name}-{}-{}",
            std::process::id(),
            std::time::SystemTime::now()
                .duration_since(std::time::UNIX_EPOCH)
                .unwrap()
                .as_nanos()
        ));
        let _ = fs::remove_dir_all(&dir);
        dir
    }

    #[test]
    fn add_folder_scans_into_tree() {
        let root = unique_dir("music");
        let album = root.join("Miles Davis").join("Kind of Blue");
        fs::create_dir_all(&album).unwrap();
        fs::write(album.join("01 So What.flac"), b"").unwrap();
        fs::write(album.join("02 Freddie.flac"), b"").unwrap();

        let data = unique_dir("data");
        fs::create_dir_all(&data).unwrap();
        let mut state = CollectionState {
            db_path: data.join("orange.db"),
            ..CollectionState::default()
        };
        let report = state.add_folder(&root.display().to_string()).unwrap();
        assert_eq!(report.songs, 2);
        assert_eq!(state.directories.len(), 1);
        let tree = state.tree();
        assert_eq!(tree.len(), 1);
        assert_eq!(tree[0].name, "Miles Davis");
        assert_eq!(tree[0].albums[0].songs.len(), 2);
        assert_eq!(tree[0].albums[0].songs[0].title, "So What");

        state.search = "freddie".into();
        assert_eq!(state.tree()[0].albums[0].songs.len(), 1);

        let id = state.directories[0].id;
        state.remove_folder(id).unwrap();
        assert!(state.songs.is_empty());
        let _ = fs::remove_dir_all(&root);
        let _ = fs::remove_dir_all(&data);
    }

    #[test]
    fn smart_playlists_filter_collection() {
        let songs = vec![
            Song {
                title: "A".into(),
                playcount: 0,
                rating: 0.8,
                ..Song::default()
            },
            Song {
                title: "B".into(),
                playcount: 12,
                rating: 0.2,
                ..Song::default()
            },
        ];
        assert_eq!(smart_never_played(&songs).len(), 1);
        assert_eq!(smart_highest_rated(&songs)[0].title, "A");
        assert_eq!(smart_most_played(&songs)[0].title, "B");
    }

    fn tagged(artist: &str, album: &str, title: &str, genre: &str, track: i64) -> Song {
        Song {
            artist: artist.into(),
            albumartist: artist.into(),
            album: album.into(),
            title: title.into(),
            genre: genre.into(),
            track,
            length_ns: 60_000_000_000,
            url: format!("file:///m/{title}.flac"),
            ..Song::default()
        }
    }

    #[test]
    fn rhythmbox_browser_narrows_genre_then_artist() {
        let songs = vec![
            tagged("Miles Davis", "Kind of Blue", "So What", "Jazz", 1),
            tagged("Miles Davis", "Kind of Blue", "Freddie", "Jazz", 2),
            tagged("John Coltrane", "Giant Steps", "Giant Steps", "Jazz", 1),
            tagged("The Beatles", "Abbey Road", "Come Together", "Rock", 1),
        ];
        let mut browser = LibraryBrowser::default();
        let genres = browser.genres(&songs, "");
        assert_eq!(genres, vec![("Jazz".into(), 3), ("Rock".into(), 1)]);
        assert_eq!(browser.tracks(&songs, "").len(), 4);

        browser.select_genre(Some("Jazz".into()));
        let artists: Vec<_> = browser
            .artists(&songs, "")
            .into_iter()
            .map(|(n, _)| n)
            .collect();
        assert_eq!(artists, vec!["John Coltrane", "Miles Davis"]);
        assert_eq!(browser.tracks(&songs, "").len(), 3);

        browser.select_artist(Some("Miles Davis".into()));
        let albums: Vec<_> = browser
            .albums(&songs, "")
            .into_iter()
            .map(|(n, _)| n)
            .collect();
        assert_eq!(albums, vec!["Kind of Blue"]);
        let tracks = browser.tracks(&songs, "");
        assert_eq!(tracks.len(), 2);
        assert_eq!(tracks[0].title, "So What");
        assert_eq!(track_list_summary(&tracks), "2 songs, 2:00");

        browser.select_genre(Some("Rock".into()));
        assert!(browser.artist.is_none());
        assert_eq!(browser.tracks(&songs, "").len(), 1);
    }
}
