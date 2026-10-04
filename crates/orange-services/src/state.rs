use crate::settings::Settings;
use orange_core::song::Song;
use orange_db::library::{MusicDirectory, SavedPlaylist};
use orange_media::playback::{EngineState, QueuedTrack};
use std::sync::Arc;

#[derive(Debug, Clone)]
pub struct Snapshot {
    pub songs: Arc<Vec<Song>>,
    pub files_path: Option<std::path::PathBuf>,
    pub files: Arc<Vec<crate::files::FsEntry>>,
    pub directories: Vec<MusicDirectory>,
    pub playlists: Vec<SavedPlaylist>,
    pub queue: Arc<Vec<QueuedTrack>>,
    pub cursor: Option<usize>,
    pub playback: EngineState,
    pub position: u64,
    pub duration: u64,
    pub spectrum: Vec<f32>,
    pub settings: Settings,
    pub lyrics: Option<(String, String)>,
    pub radio_results: Vec<crate::settings::Station>,
    pub shutdown: bool,
    pub status: String,
    pub error: Option<String>,
    pub busy: bool,
    pub can_cancel: bool,
    pub can_undo: bool,
    pub can_redo: bool,
}
impl Default for Snapshot {
    fn default() -> Self {
        Self {
            songs: Arc::new(vec![]),
            files_path: None,
            files: Arc::new(vec![]),
            directories: vec![],
            playlists: vec![],
            queue: Arc::new(vec![]),
            cursor: None,
            playback: EngineState::Empty,
            position: 0,
            duration: 0,
            spectrum: vec![],
            settings: Settings::default(),
            lyrics: None,
            radio_results: vec![],
            shutdown: false,
            status: "Opening collection…".into(),
            error: None,
            busy: false,
            can_cancel: false,
            can_undo: false,
            can_redo: false,
        }
    }
}
impl Snapshot {
    pub fn current(&self) -> Option<&QueuedTrack> {
        self.cursor.and_then(|i| self.queue.get(i))
    }
}
