//! Shared application commands, independent of rendering or native menus.
use orange_core::song::Song;
use std::path::PathBuf;
#[derive(Debug, Clone)]
pub enum Action {
    PlayPause,
    Stop,
    Next,
    Previous,
    Seek(u64),
    Volume(u8),
    StopAfterCurrent,
    Play(Vec<Song>, usize),
    PlayShared(std::sync::Arc<Vec<Song>>, usize),
    Enqueue(Vec<Song>),
    PlayQueue(usize),
    RemoveQueue(usize),
    ClearQueue,
    Repeat(u8),
    Shuffle(u8),
    Equalizer(usize, f64),
    Theme(String),
    Window(u32, u32),
    AddFolder(PathBuf),
    RemoveFolder(i64),
    Rescan,
    LoadPlaylist(i64),
    SavePlaylist(String),
    DeletePlaylist(i64),
    OpenFiles(Vec<PathBuf>),
    PlayFolder(PathBuf),
    BrowseFolder(PathBuf),
    FolderListed(PathBuf, Vec<crate::files::FsEntry>),
    ExportPlaylist(PathBuf),
    ImportPlaylist(PathBuf),
    AddStation(String, String),
    RemoveStation(usize),
    UndoQueue,
    RedoQueue,
    #[cfg(feature = "tags")]
    SaveTags(PathBuf, orange_media::tagger::TagPatch),
    #[cfg(feature = "gst")]
    Transcode(PathBuf, String, PathBuf),
    Sync(PathBuf),
    Cancel,
    #[cfg(feature = "online")]
    SearchRadio(String),
    #[cfg(feature = "online")]
    FetchLyrics,
    #[cfg(feature = "online")]
    RadioResults(Vec<crate::settings::Station>),
    #[cfg(feature = "online")]
    LyricsResult(String, String),
    ReportError(String),
    Rate(String, f64),
    DismissError,
    Shutdown,
    JobFinished(u64, Result<String, String>),
}
