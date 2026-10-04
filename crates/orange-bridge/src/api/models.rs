//! Owned boundary values, not a second implementation of the domain model.
#[derive(Debug, Clone)]
pub struct ConversionTargetDto {
    pub name: String,
    pub extension: String,
    pub available: bool,
}
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum FailureKind {
    Validation,
    Permission,
    CorruptData,
    NotFound,
    Network,
    Cancelled,
    Closed,
    Unsupported,
    Internal,
}

#[derive(Debug, Clone)]
pub struct BridgeFailure {
    pub kind: FailureKind,
    pub message: String,
}
impl std::fmt::Display for BridgeFailure {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        f.write_str(&self.message)
    }
}
impl std::error::Error for BridgeFailure {}
impl BridgeFailure {
    pub(crate) fn new(kind: FailureKind, message: impl Into<String>) -> Self {
        Self {
            kind,
            message: message.into(),
        }
    }
}

#[derive(Debug, Clone, PartialEq, Eq)]
pub enum SmartView {
    All,
    TopRated,
    RecentlyAdded,
    RecentlyPlayed,
    NeverPlayed,
    MostPlayed,
}

#[derive(Debug, Clone)]
pub struct LibraryQuery {
    pub text: String,
    pub genre: String,
    pub artist: String,
    pub album: String,
    pub smart: SmartView,
    pub offset: u32,
    pub limit: u32,
}
impl Default for LibraryQuery {
    fn default() -> Self {
        Self {
            text: String::new(),
            genre: String::new(),
            artist: String::new(),
            album: String::new(),
            smart: SmartView::All,
            offset: 0,
            limit: 200,
        }
    }
}

#[derive(Debug, Clone)]
pub struct TrackDto {
    pub url: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub genre: String,
    pub year: i32,
    pub track: i32,
    pub duration: u32,
    pub rating: f64,
}
#[derive(Debug, Clone)]
pub struct LibraryPage {
    pub tracks: Vec<TrackDto>,
    pub total: u32,
    pub revision: u64,
    pub genres: Vec<String>,
    pub artists: Vec<String>,
    pub albums: Vec<String>,
}
#[derive(Debug, Clone)]
pub struct NamedItem {
    pub id: i64,
    pub name: String,
}
#[derive(Debug, Clone)]
pub struct StationDto {
    pub name: String,
    pub url: String,
}
#[derive(Debug, Clone)]
pub struct FileEntryDto {
    pub name: String,
    pub path: String,
    pub directory: bool,
}
#[derive(Debug, Clone)]
pub struct CatalogDto {
    pub directories: Vec<NamedItem>,
    pub playlists: Vec<NamedItem>,
    pub stations: Vec<StationDto>,
    pub radio_results: Vec<StationDto>,
    pub files_path: Option<String>,
    pub files: Vec<FileEntryDto>,
    pub lyrics: Option<String>,
}
#[derive(Debug, Clone)]
pub struct PreferencesDto {
    pub theme: String,
    pub volume: u8,
    pub equalizer: Vec<f64>,
    pub repeat: u8,
    pub shuffle: u8,
    pub window_width: u32,
    pub window_height: u32,
}
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum PlaybackState {
    Empty,
    Stopped,
    Playing,
    Paused,
}
#[derive(Debug, Clone)]
pub struct PlaybackDto {
    pub current: Option<TrackDto>,
    pub cursor: Option<u32>,
    pub state: PlaybackState,
    pub position: u32,
    pub duration: u32,
    pub spectrum: Vec<f32>,
    pub library_revision: u64,
    pub queue_revision: u64,
    pub catalog_revision: u64,
    pub preferences: PreferencesDto,
    pub busy: bool,
    pub can_cancel: bool,
    pub can_undo: bool,
    pub can_redo: bool,
    pub status: String,
    pub error: Option<String>,
    pub shutdown: bool,
}

#[derive(Debug, Clone)]
pub struct TagEdit {
    pub path: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub genre: String,
    pub year: Option<u32>,
    pub track: Option<u32>,
}

#[derive(Debug, Clone)]
pub enum Command {
    PlayPause,
    Stop,
    Next,
    Previous,
    StopAfterCurrent,
    Seek {
        seconds: u32,
    },
    SetVolume {
        volume: u8,
    },
    PlayLibrary {
        query: LibraryQuery,
        index: u32,
    },
    Enqueue {
        urls: Vec<String>,
    },
    PlayQueue {
        index: u32,
    },
    RemoveQueue {
        index: u32,
    },
    ClearQueue,
    SetRepeat {
        mode: u8,
    },
    SetShuffle {
        mode: u8,
    },
    SetEqualizer {
        band: u8,
        gain: f64,
    },
    SetTheme {
        theme: String,
    },
    SaveWindowSize {
        width: u32,
        height: u32,
    },
    AddFolder {
        path: String,
    },
    RemoveFolder {
        id: i64,
    },
    Rescan,
    LoadPlaylist {
        id: i64,
    },
    SavePlaylist {
        name: String,
    },
    DeletePlaylist {
        id: i64,
    },
    OpenFiles {
        paths: Vec<String>,
    },
    OpenUris {
        uris: Vec<String>,
    },
    PlayFolder {
        path: String,
    },
    BrowseFolder {
        path: String,
    },
    ExportPlaylist {
        path: String,
    },
    ImportPlaylist {
        path: String,
    },
    AddStation {
        name: String,
        url: String,
    },
    RemoveStation {
        index: u32,
    },
    UndoQueue,
    RedoQueue,
    SaveTags {
        edit: TagEdit,
    },
    ConvertAudio {
        source: String,
        format: String,
        destination: String,
    },
    CopyQueue {
        destination: String,
    },
    Cancel,
    SearchRadio {
        text: String,
    },
    FetchLyrics,
    Rate {
        url: String,
        rating: f64,
    },
    DismissError,
}
