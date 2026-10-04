//! Session lifetime is owned by Rust; Dart disposes subscriptions before close.
use super::models::*;
use flutter_rust_bridge::frb;
use orange_core::song::Song;
use orange_services::{commands::Action, service::Handle, state::Snapshot};
use std::{
    path::{Path, PathBuf},
    sync::{
        atomic::{AtomicBool, Ordering},
        Arc, Mutex,
    },
    time::Duration,
};

struct Cache {
    snapshot: Snapshot,
    library_revision: u64,
    queue_revision: u64,
    catalog_revision: u64,
}

/// Capabilities come from the same domain table and factories as conversion.
pub fn conversion_targets() -> Vec<ConversionTargetDto> {
    orange_core::codecs::TRANSCODE_TARGETS
        .iter()
        .map(|target| {
            #[cfg(feature = "native")]
            let available =
                orange_media::backend_gst::available_encoder_chain(target.name).is_some();
            #[cfg(not(feature = "native"))]
            let available = false;
            ConversionTargetDto {
                name: target.name.into(),
                extension: target.extension.into(),
                available,
            }
        })
        .collect()
}

pub fn supported_audio_extensions() -> Vec<String> {
    let mut extensions: Vec<_> = orange_core::codecs::SUPPORTED_CODECS
        .iter()
        .flat_map(|codec| {
            codec
                .extensions
                .iter()
                .map(|extension| (*extension).to_owned())
        })
        .collect();
    extensions.sort();
    extensions.dedup();
    extensions
}

#[frb(opaque)]
pub struct Session {
    handle: Handle,
    cache: Mutex<Cache>,
    closed: AtomicBool,
}

pub fn open_session(
    profile: Option<String>,
    launch_uris: Vec<String>,
) -> Result<Session, BridgeFailure> {
    let _ = tracing_subscriber::fmt()
        .with_env_filter(
            tracing_subscriber::EnvFilter::try_from_default_env().unwrap_or_else(|_| "warn".into()),
        )
        .with_target(true)
        .try_init();
    let (db, settings) = match profile {
        Some(root) => {
            let root = absolute(&root)?;
            (
                root.join("data/orange/orange/orange.db"),
                root.join("config/orange/desktop.json"),
            )
        }
        None => (
            orange_core::identity::collection_db_path(orange_core::paths::data_home()),
            orange_services::settings::Settings::path(),
        ),
    };
    #[cfg(feature = "native")]
    orange_media::backend_gst::prepare_bundled_runtime();
    if let Some(parent) = db.parent() {
        std::fs::create_dir_all(parent)
            .map_err(|e| BridgeFailure::new(FailureKind::Permission, e.to_string()))?;
    }
    let handle = orange_services::service::start(db, settings, launch_uris)
        .map_err(|e| BridgeFailure::new(FailureKind::Internal, e))?;
    Ok(Session {
        handle,
        cache: Mutex::new(Cache {
            snapshot: Snapshot::default(),
            library_revision: 0,
            queue_revision: 0,
            catalog_revision: 0,
        }),
        closed: AtomicBool::new(false),
    })
}

impl Session {
    /// Small playback/preferences updates; library and queue rows stay separate.
    pub fn playback(&self) -> Result<PlaybackDto, BridgeFailure> {
        self.with_cache(|cache| {
            let s = &cache.snapshot;
            let state = match s.playback {
                orange_media::playback::EngineState::Empty => PlaybackState::Empty,
                orange_media::playback::EngineState::Playing => PlaybackState::Playing,
                orange_media::playback::EngineState::Paused => PlaybackState::Paused,
                _ => PlaybackState::Stopped,
            };
            Ok(PlaybackDto {
                current: s.current().map(queue_track),
                cursor: s.cursor.map(|i| i as u32),
                state,
                position: s.position.min(u32::MAX as u64) as u32,
                duration: s.duration.min(u32::MAX as u64) as u32,
                spectrum: s.spectrum.clone(),
                library_revision: cache.library_revision,
                queue_revision: cache.queue_revision,
                catalog_revision: cache.catalog_revision,
                preferences: PreferencesDto {
                    theme: s.settings.theme.clone(),
                    volume: s.settings.volume,
                    equalizer: s.settings.equalizer.to_vec(),
                    repeat: s.settings.repeat,
                    shuffle: s.settings.shuffle,
                    window_width: s.settings.window_width,
                    window_height: s.settings.window_height,
                },
                busy: s.busy,
                can_cancel: s.can_cancel,
                can_undo: s.can_undo,
                can_redo: s.can_redo,
                status: s.status.clone(),
                error: s.error.clone(),
                shutdown: s.shutdown,
            })
        })
    }

    pub fn query_library(&self, query: LibraryQuery) -> Result<LibraryPage, BridgeFailure> {
        validate_query(&query)?;
        self.with_cache(|cache| {
            let source = &cache.snapshot.songs;
            let songs = select_songs(source, &query);
            let distinct = |values: Vec<String>| {
                values
                    .into_iter()
                    .filter(|s| !s.is_empty())
                    .collect::<std::collections::BTreeSet<_>>()
                    .into_iter()
                    .collect()
            };
            Ok(LibraryPage {
                total: songs.len().min(u32::MAX as usize) as u32,
                revision: cache.library_revision,
                tracks: songs
                    .iter()
                    .skip(query.offset as usize)
                    .take(query.limit as usize)
                    .map(song_track)
                    .collect(),
                genres: distinct(source.iter().map(|s| s.genre.clone()).collect()),
                artists: distinct(
                    source
                        .iter()
                        .filter(|s| query.genre.is_empty() || s.genre == query.genre)
                        .map(|s| s.display_artist().to_owned())
                        .collect(),
                ),
                albums: distinct(
                    source
                        .iter()
                        .filter(|s| {
                            (query.genre.is_empty() || s.genre == query.genre)
                                && (query.artist.is_empty() || s.display_artist() == query.artist)
                        })
                        .map(|s| s.display_album().to_owned())
                        .collect(),
                ),
            })
        })
    }

    pub fn queue(&self) -> Result<Vec<TrackDto>, BridgeFailure> {
        self.with_cache(|cache| Ok(cache.snapshot.queue.iter().map(queue_track).collect()))
    }

    pub fn catalog(&self) -> Result<CatalogDto, BridgeFailure> {
        self.with_cache(|cache| {
            let s = &cache.snapshot;
            Ok(CatalogDto {
                directories: s
                    .directories
                    .iter()
                    .map(|d| NamedItem {
                        id: d.id,
                        name: d.path.clone(),
                    })
                    .collect(),
                playlists: s
                    .playlists
                    .iter()
                    .map(|p| NamedItem {
                        id: p.id,
                        name: p.name.clone(),
                    })
                    .collect(),
                stations: s
                    .settings
                    .stations
                    .iter()
                    .map(|r| StationDto {
                        name: r.name.clone(),
                        url: r.url.clone(),
                    })
                    .collect(),
                radio_results: s
                    .radio_results
                    .iter()
                    .map(|r| StationDto {
                        name: r.name.clone(),
                        url: r.url.clone(),
                    })
                    .collect(),
                files_path: s
                    .files_path
                    .as_ref()
                    .map(|p| p.to_string_lossy().into_owned()),
                files: s
                    .files
                    .iter()
                    .map(|f| FileEntryDto {
                        name: f.name.clone(),
                        path: f.path.to_string_lossy().into_owned(),
                        directory: f.is_dir,
                    })
                    .collect(),
                lyrics: s.lyrics.as_ref().map(|(_, text)| text.clone()),
            })
        })
    }

    /// A shared command path for menus, keyboard actions and rendered controls.
    pub fn dispatch(&self, command: Command) -> Result<(), BridgeFailure> {
        self.with_cache(|cache| {
            let validation = |s: &str| BridgeFailure::new(FailureKind::Validation, s);
            let action = match command {
                Command::PlayPause => Action::PlayPause,
                Command::Stop => Action::Stop,
                Command::Next => Action::Next,
                Command::Previous => Action::Previous,
                Command::StopAfterCurrent => Action::StopAfterCurrent,
                Command::Seek { seconds } => Action::Seek(seconds as u64),
                Command::SetVolume { volume } if volume <= 100 => Action::Volume(volume),
                Command::SetVolume { .. } => {
                    return Err(validation("Volume must be between 0 and 100."))
                }
                Command::PlayLibrary { query, index } => {
                    validate_query(&query)?;
                    let songs = select_songs(&cache.snapshot.songs, &query);
                    if index as usize >= songs.len() {
                        return Err(validation(
                            "This track is no longer in the selected collection.",
                        ));
                    }
                    Action::Play(songs, index as usize)
                }
                Command::Enqueue { urls } => {
                    if urls.len() > 50_000 {
                        return Err(validation("Too many tracks selected."));
                    }
                    let songs = urls
                        .iter()
                        .map(|url| {
                            cache
                                .snapshot
                                .songs
                                .iter()
                                .find(|s| &s.url == url)
                                .cloned()
                                .ok_or_else(|| {
                                    BridgeFailure::new(
                                        FailureKind::NotFound,
                                        "This track is no longer in the library.",
                                    )
                                })
                        })
                        .collect::<Result<Vec<_>, _>>()?;
                    Action::Enqueue(songs)
                }
                Command::PlayQueue { index } | Command::RemoveQueue { index }
                    if index as usize >= cache.snapshot.queue.len() =>
                {
                    return Err(validation("This queue row no longer exists."))
                }
                Command::PlayQueue { index } => Action::PlayQueue(index as usize),
                Command::RemoveQueue { index } => Action::RemoveQueue(index as usize),
                Command::ClearQueue => Action::ClearQueue,
                Command::SetRepeat { mode } if mode <= 3 => Action::Repeat(mode),
                Command::SetShuffle { mode } if mode <= 2 => Action::Shuffle(mode),
                Command::SetRepeat { .. } | Command::SetShuffle { .. } => {
                    return Err(validation("Unsupported playback mode."))
                }
                Command::SetEqualizer { band, gain }
                    if band < 10 && gain.is_finite() && (-12.0..=12.0).contains(&gain) =>
                {
                    Action::Equalizer(band as usize, gain)
                }
                Command::SetEqualizer { .. } => {
                    return Err(validation("Equalizer gains must be between -12 and 12 dB."))
                }
                Command::SetTheme { theme }
                    if ["system", "light", "dark"].contains(&theme.as_str()) =>
                {
                    Action::Theme(theme)
                }
                Command::SetTheme { .. } => return Err(validation("Unknown appearance choice.")),
                Command::SaveWindowSize { width, height } => Action::Window(width, height),
                Command::AddFolder { path } => Action::AddFolder(absolute(&path)?),
                Command::RemoveFolder { id } => Action::RemoveFolder(id),
                Command::Rescan => Action::Rescan,
                Command::LoadPlaylist { id } => Action::LoadPlaylist(id),
                Command::SavePlaylist { name } => Action::SavePlaylist(name),
                Command::DeletePlaylist { id } => Action::DeletePlaylist(id),
                Command::OpenFiles { paths } => Action::OpenFiles(
                    paths
                        .iter()
                        .map(|p| absolute(p))
                        .collect::<Result<Vec<_>, _>>()?,
                ),
                Command::OpenUris { uris } => Action::OpenUris(uris),
                Command::PlayFolder { path } => Action::PlayFolder(absolute(&path)?),
                Command::BrowseFolder { path } => Action::BrowseFolder(absolute(&path)?),
                Command::ExportPlaylist { path } => Action::ExportPlaylist(absolute(&path)?),
                Command::ImportPlaylist { path } => Action::ImportPlaylist(absolute(&path)?),
                Command::AddStation { name, url } => {
                    if orange_media::radio::validate_custom_stream(&name, &url).is_none() {
                        return Err(validation(
                            "Enter a station name and a valid HTTP(S) URL without credentials.",
                        ));
                    }
                    Action::AddStation(name, url)
                }
                Command::RemoveStation { index } => Action::RemoveStation(index as usize),
                Command::UndoQueue => Action::UndoQueue,
                Command::RedoQueue => Action::RedoQueue,
                #[cfg(feature = "native")]
                Command::SaveTags { edit } => Action::SaveTags(
                    absolute(&edit.path)?,
                    orange_media::tagger::TagPatch {
                        title: Some(edit.title),
                        artist: Some(edit.artist),
                        album: Some(edit.album),
                        genre: Some(edit.genre),
                        year: edit.year,
                        track: edit.track,
                        disc: None,
                    },
                ),
                #[cfg(feature = "native")]
                Command::ConvertAudio {
                    source,
                    format,
                    destination,
                } => Action::Transcode(absolute(&source)?, format, absolute(&destination)?),
                #[cfg(feature = "native")]
                Command::SearchRadio { text } => Action::SearchRadio(text),
                #[cfg(feature = "native")]
                Command::FetchLyrics => Action::FetchLyrics,
                #[cfg(not(feature = "native"))]
                Command::SaveTags { .. }
                | Command::ConvertAudio { .. }
                | Command::SearchRadio { .. }
                | Command::FetchLyrics => {
                    return Err(BridgeFailure::new(
                        FailureKind::Unsupported,
                        "This operation requires a native media build.",
                    ))
                }
                Command::CopyQueue { destination } => Action::Sync(absolute(&destination)?),
                Command::Cancel => Action::Cancel,
                Command::Rate { url, rating }
                    if rating.is_finite() && (0.0..=1.0).contains(&rating) =>
                {
                    Action::Rate(url, rating)
                }
                Command::Rate { .. } => return Err(validation("Rating must be between 0 and 1.")),
                Command::DismissError => Action::DismissError,
            };
            self.handle
                .dispatch(action)
                .map_err(|e| BridgeFailure::new(FailureKind::Closed, e))
        })
    }

    pub fn close(&self) -> Result<(), BridgeFailure> {
        self.closed.store(true, Ordering::Release);
        self.handle
            .shutdown(Duration::from_secs(10))
            .map_err(|e| BridgeFailure::new(FailureKind::Internal, e))
    }

    #[frb(ignore)]
    fn with_cache<T>(
        &self,
        work: impl FnOnce(&Cache) -> Result<T, BridgeFailure>,
    ) -> Result<T, BridgeFailure> {
        if self.closed.load(Ordering::Acquire) {
            return Err(BridgeFailure::new(
                FailureKind::Closed,
                "The session has closed.",
            ));
        }
        let mut cache = self.cache.lock().map_err(|_| {
            BridgeFailure::new(FailureKind::Internal, "Session state is unavailable.")
        })?;
        if let Some(next) = self.handle.poll() {
            if !Arc::ptr_eq(&cache.snapshot.songs, &next.songs) {
                cache.library_revision += 1;
            }
            if !Arc::ptr_eq(&cache.snapshot.queue, &next.queue) {
                cache.queue_revision += 1;
            }
            if cache.snapshot.directories != next.directories
                || cache.snapshot.playlists != next.playlists
                || cache.snapshot.settings.stations != next.settings.stations
                || cache.snapshot.radio_results != next.radio_results
                || !Arc::ptr_eq(&cache.snapshot.files, &next.files)
                || cache.snapshot.lyrics != next.lyrics
            {
                cache.catalog_revision += 1;
            }
            cache.snapshot = next;
        }
        work(&cache)
    }
}
impl Drop for Session {
    fn drop(&mut self) {
        let _ = self.close();
    }
}

pub fn radio_presets() -> Vec<StationDto> {
    [
        orange_media::radio::radio_paradise(),
        orange_media::radio::somafm(),
    ]
    .into_iter()
    .flat_map(|service| {
        service.streams.into_iter().map(move |station| StationDto {
            name: format!("{} · {}", service.name, station.name),
            url: station.url,
        })
    })
    .collect()
}

fn absolute(value: &str) -> Result<PathBuf, BridgeFailure> {
    let path = Path::new(value);
    if !path.is_absolute() {
        return Err(BridgeFailure::new(
            FailureKind::Validation,
            "Choose an absolute filesystem path.",
        ));
    }
    Ok(path.to_owned())
}
fn validate_query(query: &LibraryQuery) -> Result<(), BridgeFailure> {
    if query.text.len() > 4096 || query.limit == 0 || query.limit > 1000 {
        return Err(BridgeFailure::new(
            FailureKind::Validation,
            "Search is too long or the page size is invalid.",
        ));
    }
    Ok(())
}
fn select_songs(source: &[Song], query: &LibraryQuery) -> Vec<Song> {
    use orange_services::library::*;
    let base = match query.smart {
        SmartView::All => source.to_vec(),
        SmartView::TopRated => smart_highest_rated(source),
        SmartView::RecentlyAdded => smart_recently_added(source),
        SmartView::RecentlyPlayed => smart_recently_played(source),
        SmartView::NeverPlayed => smart_never_played(source),
        SmartView::MostPlayed => smart_most_played(source),
    };
    let filter = orange_collection::filter::CollectionFilter::parse(&query.text);
    base.into_iter()
        .filter(|s| {
            filter.matches(s)
                && (query.genre.is_empty() || s.genre == query.genre)
                && (query.artist.is_empty() || s.display_artist() == query.artist)
                && (query.album.is_empty() || s.display_album() == query.album)
        })
        .collect()
}
fn song_track(song: &Song) -> TrackDto {
    TrackDto {
        url: song.url.clone(),
        title: song.display_title(),
        artist: song.display_artist().to_owned(),
        album: song.display_album().to_owned(),
        genre: song.genre.clone(),
        year: song.year.clamp(i32::MIN as i64, i32::MAX as i64) as i32,
        track: song.track.clamp(i32::MIN as i64, i32::MAX as i64) as i32,
        duration: song.length_secs().clamp(0, u32::MAX as i64) as u32,
        rating: song.rating,
    }
}
fn queue_track(track: &orange_media::playback::QueuedTrack) -> TrackDto {
    song_track(&Song {
        url: track.url.clone(),
        title: track.title.clone(),
        artist: track.artist.clone(),
        album: track.album.clone(),
        genre: track.genre.clone(),
        year: track.year,
        track: track.track,
        length_ns: track.length_ns,
        ..Song::default()
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    fn ready(session: &Session) {
        for _ in 0..100 {
            if session.playback().unwrap().library_revision > 0 {
                return;
            }
            std::thread::sleep(Duration::from_millis(20));
        }
        panic!("Service did not open");
    }
    #[test]
    fn unicode_profile_persistence_and_closed_session_failures() {
        let temp = tempfile::tempdir().unwrap();
        let profile = temp
            .path()
            .join("Música 日本")
            .to_string_lossy()
            .into_owned();
        let session = open_session(Some(profile.clone()), vec![]).unwrap();
        ready(&session);
        session
            .dispatch(Command::SetTheme {
                theme: "dark".into(),
            })
            .unwrap();
        session.dispatch(Command::SetVolume { volume: 37 }).unwrap();
        session.close().unwrap();
        session.close().unwrap();
        assert_eq!(session.playback().unwrap_err().kind, FailureKind::Closed);
        let session = open_session(Some(profile), vec![]).unwrap();
        ready(&session);
        let p = session.playback().unwrap().preferences;
        assert_eq!((p.theme.as_str(), p.volume), ("dark", 37));
        session.close().unwrap();
    }
    #[test]
    fn invalid_inputs_fail_before_dispatch_and_queries_are_bounded() {
        assert!(matches!(
            open_session(Some("relative".into()), vec![]),
            Err(BridgeFailure {
                kind: FailureKind::Validation,
                ..
            })
        ));
        let temp = tempfile::tempdir().unwrap();
        let session =
            open_session(Some(temp.path().to_string_lossy().into_owned()), vec![]).unwrap();
        ready(&session);
        assert_eq!(
            session
                .dispatch(Command::SetVolume { volume: 101 })
                .unwrap_err()
                .kind,
            FailureKind::Validation
        );
        assert_eq!(
            session
                .dispatch(Command::SetEqualizer {
                    band: 0,
                    gain: f64::NAN
                })
                .unwrap_err()
                .kind,
            FailureKind::Validation
        );
        assert!(session
            .query_library(LibraryQuery {
                limit: 1001,
                ..LibraryQuery::default()
            })
            .is_err());
        assert!(session.dispatch(Command::PlayQueue { index: 0 }).is_err());
        let a = session.playback().unwrap();
        let b = session.playback().unwrap();
        assert_eq!(a.library_revision, b.library_revision);
        session.close().unwrap();
    }
    #[test]
    fn domain_queries_preserve_unicode_filters_and_smart_rules() {
        let songs = vec![
            Song {
                title: "日本".into(),
                artist: "Miles".into(),
                genre: "Jazz".into(),
                rating: 0.8,
                ..Song::default()
            },
            Song {
                title: "Else".into(),
                artist: "Other".into(),
                ..Song::default()
            },
        ];
        let query = LibraryQuery {
            text: "日本".into(),
            genre: "Jazz".into(),
            smart: SmartView::TopRated,
            ..LibraryQuery::default()
        };
        let selected = select_songs(&songs, &query);
        assert_eq!(selected.len(), 1);
        assert_eq!(selected[0].title, "日本");
    }
}
