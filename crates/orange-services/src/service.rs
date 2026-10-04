//! A single command owner for playback; long I/O runs in cancellable jobs.
use crate::{
    commands::Action,
    library::CollectionState,
    settings::{Settings, Station, Track},
    state::Snapshot,
};
use orange_core::song::Song;
use orange_media::playback::{EngineState, Player, QueuedTrack, StopBehaviour};
use std::path::{Path, PathBuf};
use std::sync::{
    atomic::{AtomicBool, Ordering},
    mpsc, Arc, Condvar, Mutex,
};
use std::time::{Duration, Instant, SystemTime, UNIX_EPOCH};

#[derive(Clone)]
pub struct Handle {
    commands: mpsc::Sender<Action>,
    updates: Arc<Mutex<Option<Snapshot>>>,
    finished: Arc<(Mutex<bool>, Condvar)>,
}
impl Handle {
    pub fn dispatch(&self, action: Action) -> Result<(), String> {
        self.commands
            .send(action)
            .map_err(|_| "The application service has stopped.".into())
    }
    /// Complete settings/audio shutdown before the desktop event loop exits.
    pub fn shutdown(&self, timeout: Duration) -> Result<(), String> {
        let (state, condition) = &*self.finished;
        let completed = state.lock().map_err(|e| e.to_string())?;
        if *completed {
            return Ok(());
        }
        self.dispatch(Action::Shutdown)?;
        let (completed, _) = condition
            .wait_timeout_while(completed, timeout, |done| !*done)
            .map_err(|e| e.to_string())?;
        if *completed {
            Ok(())
        } else {
            Err("Application shutdown timed out".into())
        }
    }
    pub fn poll(&self) -> Option<Snapshot> {
        self.updates.lock().ok()?.take()
    }
}
pub fn start(db: PathBuf, settings_path: PathBuf, uris: Vec<String>) -> Result<Handle, String> {
    let (tx, rx) = mpsc::channel();
    let updates = Arc::new(Mutex::new(None));
    let out = updates.clone();
    let loop_tx = tx.clone();
    let finished = Arc::new((Mutex::new(false), Condvar::new()));
    let completed = finished.clone();
    std::thread::Builder::new()
        .name("orange-services".into())
        .spawn(move || {
            let mut worker = Worker::new(db, settings_path, loop_tx, out);
            if !uris.is_empty() {
                worker.apply(Action::OpenUris(uris));
            }
            worker.publish();
            while !worker.shutdown {
                match rx.recv_timeout(Duration::from_millis(100)) {
                    Ok(action) => worker.apply(action),
                    Err(mpsc::RecvTimeoutError::Timeout) => {}
                    Err(_) => break,
                }
                worker.tick();
                worker.publish();
            }
            worker.cancel.store(true, Ordering::Relaxed);
            if let Some(job) = worker.job_thread.take() {
                if job.join().is_err() {
                    worker.fail("A background operation ended unexpectedly during shutdown.");
                }
            }
            worker.persist();
            #[cfg(feature = "gst")]
            if let Some(engine) = worker.engine.take() {
                if let Err(error) = engine.stop() {
                    tracing::warn!("Audio shutdown failed: {error}");
                }
            }
            let (state, condition) = &*completed;
            if let Ok(mut done) = state.lock() {
                *done = true;
                condition.notify_all();
            }
        })
        .map_err(|e| e.to_string())?;
    Ok(Handle {
        commands: tx,
        updates,
        finished,
    })
}

struct Worker {
    collection: CollectionState,
    player: Player,
    snapshot: Snapshot,
    settings_path: PathBuf,
    settings_writable: bool,
    commands: mpsc::Sender<Action>,
    updates: Arc<Mutex<Option<Snapshot>>>,
    cancel: Arc<AtomicBool>,
    job: u64,
    job_thread: Option<std::thread::JoinHandle<()>>,
    undo: Vec<Player>,
    redo: Vec<Player>,
    seed: u64,
    shutdown: bool,
    dirty: bool,
    queue_dirty: bool,
    settings_pending: Option<Instant>,
    #[cfg(feature = "notify")]
    notified: Option<String>,
    #[cfg(feature = "gst")]
    engine: Option<orange_media::backend_gst::GstEngine>,
    #[cfg(feature = "gst")]
    engine_url: String,
    #[cfg(all(feature = "dbus", target_os = "linux"))]
    mpris: crate::mpris_host::MprisHost,
    #[cfg(all(feature = "dbus", target_os = "linux"))]
    remotes: mpsc::Receiver<orange_media::mpris_server::MprisCommand>,
}
impl Worker {
    fn new(
        db: PathBuf,
        settings_path: PathBuf,
        commands: mpsc::Sender<Action>,
        updates: Arc<Mutex<Option<Snapshot>>>,
    ) -> Self {
        let mut snapshot = Snapshot::default();
        let settings = Settings::load(&settings_path);
        let writable = settings.is_ok();
        snapshot.settings = settings.unwrap_or_else(|error| {
            snapshot.error = Some(error);
            Settings::default()
        });
        let mut collection = CollectionState {
            db_path: db,
            ..CollectionState::default()
        };
        collection.reload();
        if let Some(error) = &collection.last_error {
            snapshot.error = Some(error.clone());
        }
        let mut player = Player::new();
        player.enqueue_many(snapshot.settings.queue.iter().cloned().map(Into::into));
        player.restore_cursor(snapshot.settings.cursor);
        player.set_volume(snapshot.settings.volume);
        #[cfg(all(feature = "dbus", target_os = "linux"))]
        let (mpris, remotes) = {
            let host = crate::mpris_host::MprisHost::new();
            let (tx, rx) = mpsc::channel();
            host.spawn_server(orange_media::mpris::BUS_NAME.into(), tx);
            (host, rx)
        };
        let mut worker = Self {
            collection,
            player,
            snapshot,
            settings_path,
            settings_writable: writable,
            commands,
            updates,
            cancel: Arc::new(AtomicBool::new(false)),
            job: 0,
            job_thread: None,
            undo: vec![],
            redo: vec![],
            seed: SystemTime::now()
                .duration_since(UNIX_EPOCH)
                .map(|d| d.as_nanos() as u64)
                .unwrap_or(1),
            shutdown: false,
            dirty: true,
            queue_dirty: true,
            settings_pending: None,
            #[cfg(feature = "notify")]
            notified: None,
            #[cfg(feature = "gst")]
            engine: None,
            #[cfg(feature = "gst")]
            engine_url: String::new(),
            #[cfg(all(feature = "dbus", target_os = "linux"))]
            mpris,
            #[cfg(all(feature = "dbus", target_os = "linux"))]
            remotes,
        };
        worker.refresh();
        worker.snapshot.status = "Ready".into();
        worker
    }
    fn refresh(&mut self) {
        self.collection.reload();
        if let Some(e) = &self.collection.last_error {
            self.snapshot.error = Some(e.clone());
        }
        self.snapshot.songs = Arc::new(self.collection.songs.clone());
        self.snapshot.directories = self.collection.directories.clone();
        self.snapshot.playlists = self.collection.playlists.clone();
        self.dirty = true;
    }
    fn history(&mut self) {
        self.undo.push(self.player.clone());
        if self.undo.len() > 20 {
            self.undo.remove(0);
        }
        self.redo.clear();
    }
    fn fail(&mut self, error: impl Into<String>) {
        self.snapshot.error = Some(error.into());
        self.snapshot.status = "Operation failed".into();
        self.dirty = true;
        tracing::warn!("Application operation failed; details are available in the error panel");
    }
    fn refresh_queue(&mut self) {
        if self.queue_dirty {
            self.snapshot.queue = Arc::new(self.player.queue().to_vec());
            self.snapshot.settings.queue =
                Arc::new(self.player.queue().iter().map(Track::from).collect());
            self.queue_dirty = false;
        }
    }
    fn persist(&mut self) {
        if !self.settings_writable {
            return;
        }
        self.refresh_queue();
        self.snapshot.settings.cursor = self.player.cursor();
        if let Err(error) = self.snapshot.settings.save(&self.settings_path) {
            self.fail(format!("Settings could not be saved: {error}"));
        }
    }
    fn job(
        &mut self,
        cancellable: bool,
        work: impl FnOnce(Arc<AtomicBool>) -> Result<String, String> + Send + 'static,
    ) {
        if self.snapshot.busy {
            self.fail("Wait for the current operation or cancel it first.");
            return;
        }
        self.job += 1;
        let id = self.job;
        self.cancel = Arc::new(AtomicBool::new(false));
        let flag = self.cancel.clone();
        let tx = self.commands.clone();
        self.snapshot.busy = true;
        self.snapshot.can_cancel = cancellable;
        self.snapshot.error = None;
        self.snapshot.status = "Working…".into();
        match std::thread::Builder::new()
            .name("orange-file-job".into())
            .spawn(move || {
                let result = work(flag);
                let _ = tx.send(Action::JobFinished(id, result));
            }) {
            Ok(thread) => self.job_thread = Some(thread),
            Err(error) => {
                self.snapshot.busy = false;
                self.fail(error.to_string());
            }
        }
    }
    fn apply(&mut self, action: Action) {
        if matches!(
            &action,
            Action::Play(..)
                | Action::PlayShared(..)
                | Action::Enqueue(..)
                | Action::RemoveQueue(_)
                | Action::ClearQueue
                | Action::UndoQueue
                | Action::RedoQueue
                | Action::LoadPlaylist(_)
                | Action::ImportPlaylist(_)
        ) {
            self.queue_dirty = true;
        }
        let mut save = true;
        self.dirty = true;
        match action {
            Action::PlayPause => self.player.toggle_play_pause(),
            Action::Stop => {
                self.player.stop();
                self.snapshot.position = 0;
            }
            Action::Next => self.advance(true),
            Action::Previous => {
                if self.player.previous() {
                    self.restart();
                } else {
                    self.snapshot.position = 0;
                    self.seek(0);
                }
            }
            Action::Seek(seconds) => {
                self.seek(seconds);
                save = false;
            }
            Action::Volume(value) => {
                self.player.set_volume(value);
                self.snapshot.settings.volume = value.min(100);
                #[cfg(feature = "gst")]
                if let Some(engine) = &self.engine {
                    if let Err(e) = engine.set_output_volume(f64::from(value.min(100)) / 100.0) {
                        self.fail(e.to_string());
                    }
                }
            }
            Action::StopAfterCurrent => self.player.set_stop_after_current(),
            Action::PlayShared(songs, index) => {
                self.apply(Action::Play(songs.as_ref().clone(), index))
            }
            Action::Play(songs, index) => {
                self.history();
                self.player
                    .replace_and_play(songs.iter().map(QueuedTrack::from_song).collect(), index);
                self.restart();
            }
            Action::Enqueue(songs) => {
                self.history();
                self.player
                    .enqueue_many(songs.iter().map(QueuedTrack::from_song));
            }
            Action::PlayQueue(index) => {
                self.player.play_at(index);
                self.restart();
            }
            Action::RemoveQueue(index) => {
                self.history();
                let removed_current = self.player.cursor() == Some(index);
                self.player.remove_at(index);
                if removed_current {
                    self.restart();
                }
            }
            Action::ClearQueue => {
                self.history();
                self.player.clear_queue();
            }
            Action::UndoQueue => {
                if let Some(p) = self.undo.pop() {
                    self.redo.push(self.player.clone());
                    self.player = p;
                }
            }
            Action::RedoQueue => {
                if let Some(p) = self.redo.pop() {
                    self.undo.push(self.player.clone());
                    self.player = p;
                }
            }
            Action::Repeat(mode) => self.snapshot.settings.repeat = mode.min(3),
            Action::Shuffle(mode) => self.snapshot.settings.shuffle = mode.min(2),
            Action::Theme(theme) => {
                self.snapshot.settings.theme =
                    orange_core::appearance::AppearanceMode::parse(&theme)
                        .as_str()
                        .into()
            }
            Action::Equalizer(band, gain) => {
                if let Some(slot) = self.snapshot.settings.equalizer.get_mut(band) {
                    *slot = if gain.is_finite() {
                        gain.clamp(-12.0, 12.0)
                    } else {
                        0.0
                    };
                }
                #[cfg(feature = "gst")]
                if let Some(engine) = &self.engine {
                    if let Err(e) = engine.set_equalizer_gain(band, gain) {
                        self.fail(e.to_string());
                    }
                }
            }
            Action::Window(width, height) => {
                self.settings_pending = Some(Instant::now());
                save = false;
                self.snapshot.settings.window_width = width.clamp(600, 7680);
                self.snapshot.settings.window_height = height.clamp(400, 4320);
            }
            Action::AddFolder(path) => {
                let mut c = self.collection.clone();
                let flag = self.cancel.clone();
                c.cancellation = Some(flag);
                self.job(true, move |flag| {
                    c.cancellation = Some(flag);
                    let report = c.add_folder(&path.to_string_lossy())?;
                    Ok(format!("Imported {} songs", report.songs))
                });
                save = false;
            }
            Action::Rescan => {
                let mut c = self.collection.clone();
                self.job(true, move |flag| {
                    c.cancellation = Some(flag);
                    let report = c.rescan_all()?;
                    Ok(format!("Rescanned {} songs", report.songs))
                });
                save = false;
            }
            Action::RemoveFolder(id) => {
                let mut c = self.collection.clone();
                self.job(false, move |_| {
                    c.remove_folder(id)?;
                    Ok("Folder removed from library; files remain on disk.".into())
                });
                save = false;
            }
            Action::LoadPlaylist(id) => match self.collection.load_saved_playlist(id) {
                Ok(songs) => {
                    self.history();
                    self.player.clear_queue();
                    self.player
                        .enqueue_many(songs.iter().map(QueuedTrack::from_song));
                    self.snapshot.status = "Playlist loaded into the queue".into();
                }
                Err(e) => self.fail(e),
            },
            Action::SavePlaylist(name) => {
                let name = name.trim().to_owned();
                if name.is_empty() || name.len() > 200 {
                    self.fail("Playlist name must contain 1–200 characters.");
                    return;
                }
                let db = self.collection.db_path.clone();
                let songs = queue_songs(&self.player);
                self.job(false, move |_| {
                    let c = orange_db::open_collection(&db, orange_db::OpenMode::ReadWrite)
                        .map_err(|e| e.to_string())?;
                    orange_db::library::save_playlist(&c, &name, &songs)
                        .map_err(|e| e.to_string())?;
                    Ok(format!("Saved playlist {name}"))
                });
                save = false;
            }
            Action::DeletePlaylist(id) => {
                let db = self.collection.db_path.clone();
                self.job(false, move |_| {
                    let c = orange_db::open_collection(&db, orange_db::OpenMode::ReadWrite)
                        .map_err(|e| e.to_string())?;
                    orange_db::library::delete_playlist(&c, id).map_err(|e| e.to_string())?;
                    Ok("Playlist deleted; music files remain on disk.".into())
                });
                save = false;
            }
            Action::OpenFiles(paths) => {
                let songs = paths
                    .iter()
                    .filter(|p| p.is_file() && orange_collection::scan::is_audio_path(p))
                    .map(|p| {
                        orange_collection::scan::song_from_path(
                            p,
                            p.parent().unwrap_or(Path::new("")),
                            -1,
                        )
                    })
                    .collect::<Vec<_>>();
                if songs.is_empty() {
                    self.fail("No supported audio files selected.");
                    return;
                }
                self.apply(Action::Play(songs, 0));
            }
            Action::OpenUris(uris) => match songs_from_uris(uris) {
                Ok(songs) if !songs.is_empty() => self.apply(Action::Play(songs, 0)),
                Ok(_) => self.fail("The selected playlist contains no tracks."),
                Err(error) => self.fail(error),
            },
            Action::BrowseFolder(path) => {
                let tx = self.commands.clone();
                self.job(false, move |_| {
                    let entries = crate::files::list_entries(&path)?;
                    tx.send(Action::FolderListed(path, entries))
                        .map_err(|error| error.to_string())?;
                    Ok("Folder opened".into())
                });
                save = false;
            }
            Action::FolderListed(path, entries) => {
                self.snapshot.files_path = Some(path);
                self.snapshot.files = Arc::new(entries);
                save = false;
            }
            Action::PlayFolder(root) => {
                let tx = self.commands.clone();
                self.job(true, move |flag| {
                    let files = orange_collection::scan::scan_directory_checked(&root, &|| {
                        flag.load(Ordering::Relaxed)
                    })
                    .map_err(|e| e.to_string())?;
                    let songs = files
                        .iter()
                        .map(|file| orange_collection::scan::song_from_scanned(file, &root, -1))
                        .collect::<Vec<_>>();
                    if songs.is_empty() {
                        return Err("This folder contains no supported audio files.".into());
                    }
                    tx.send(Action::Play(songs, 0)).map_err(|e| e.to_string())?;
                    Ok("Folder opened without adding it to the collection".into())
                });
                save = false;
            }
            Action::ImportPlaylist(path) => match read_playlist(&path) {
                Ok(songs) => {
                    self.history();
                    self.player.clear_queue();
                    self.player
                        .enqueue_many(songs.iter().map(QueuedTrack::from_song));
                    self.snapshot.status = "Playlist imported".into();
                }
                Err(e) => self.fail(e),
            },
            Action::ExportPlaylist(path) => {
                let entries = self
                    .player
                    .queue()
                    .iter()
                    .map(|t| orange_playlist::parsers::ParsedEntry {
                        url: t.url.clone(),
                        title: t.title.clone(),
                        length_secs: Some(t.length_secs()),
                    })
                    .collect::<Vec<_>>();
                self.job(false, move |_| {
                    write_new(
                        &path,
                        orange_playlist::parsers::write_m3u(&entries).as_bytes(),
                    )?;
                    Ok("Playlist exported".into())
                });
                save = false;
            }
            Action::AddStation(name, url) => {
                if let Some(stream) = orange_media::radio::validate_custom_stream(&name, &url) {
                    self.snapshot.settings.stations.push(Station {
                        name: stream.name,
                        url: stream.url,
                    });
                    self.snapshot.status = "Radio station saved".into();
                } else {
                    self.fail("Enter a station name and a valid HTTP(S) URL without credentials.");
                }
            }
            Action::RemoveStation(index) => {
                if index < self.snapshot.settings.stations.len() {
                    self.snapshot.settings.stations.remove(index);
                }
            }
            #[cfg(feature = "tags")]
            Action::SaveTags(path, patch) => {
                self.job(false, move |_| {
                    orange_media::tagger::write_tags(&path, &patch).map_err(|e| e.to_string())?;
                    Ok("Tags saved; rescan the library to update its metadata.".into())
                });
                save = false;
            }
            #[cfg(feature = "gst")]
            Action::Transcode(source, target, destination) => {
                self.job(true, move |flag| {
                    let parent = destination.parent().ok_or("Choose an output directory")?;
                    std::fs::create_dir_all(parent).map_err(|e| e.to_string())?;
                    let temp = tempfile::NamedTempFile::new_in(parent)
                        .map_err(|e| e.to_string())?
                        .into_temp_path();
                    let chain = orange_media::backend::TranscodeChain {
                        input_uri: orange_core::paths::path_to_file_url(&source),
                        target_name: target,
                        output_path: temp.to_string_lossy().into_owned(),
                    };
                    let report = orange_media::backend_gst::transcode_file_cancellable(
                        &chain,
                        Duration::from_secs(24 * 3600),
                        &|| flag.load(Ordering::Relaxed),
                    )
                    .map_err(|e| e.to_string())?;
                    if flag.load(Ordering::Relaxed) {
                        return Err("Conversion cancelled; original files are intact.".into());
                    }
                    temp.persist_noclobber(destination)
                        .map_err(|e| e.to_string())?;
                    Ok(format!("Converted {} bytes", report.bytes_written))
                });
                save = false;
            }
            Action::Sync(destination) => {
                let songs = queue_songs(&self.player);
                self.job(true, move |flag| {
                    if flag.load(Ordering::Relaxed) {
                        return Err("Copy cancelled".into());
                    }
                    let tracks = songs
                        .iter()
                        .map(|s| orange_media::devices::SyncTrack {
                            source_url: s.url.clone(),
                            artist: s.artist.clone(),
                            album: s.album.clone(),
                            title: s.title.clone(),
                            track_no: s.track.max(0) as u32,
                        })
                        .collect::<Vec<_>>();
                    let report = orange_media::devices::execute_sync_cancellable(
                        &destination,
                        &tracks,
                        false,
                        None,
                        &mut |_| {},
                        None,
                        &|| flag.load(Ordering::Relaxed),
                    )
                    .map_err(|e| e.to_string())?;
                    Ok(format!(
                        "Copied {} tracks; {} existing files skipped",
                        report.copied, report.skipped
                    ))
                });
                save = false;
            }
            Action::Cancel if self.snapshot.can_cancel => {
                self.cancel.store(true, Ordering::Relaxed);
                self.snapshot.status = "Cancellation requested…".into();
                save = false;
            }
            Action::Cancel => {
                save = false;
            }
            Action::JobFinished(id, result) => {
                if id == self.job {
                    if let Some(thread) = self.job_thread.take() {
                        if thread.join().is_err() {
                            self.fail("A background operation ended unexpectedly.");
                        }
                    }
                    self.snapshot.busy = false;
                    self.snapshot.can_cancel = false;
                    self.refresh();
                    match result {
                        Ok(status) => self.snapshot.status = status,
                        Err(error) => self.fail(error),
                    }
                }
                save = false;
            }
            #[cfg(feature = "online")]
            Action::SearchRadio(query) => {
                if query.trim().is_empty() {
                    self.fail("Enter a station name to search");
                    return;
                }
                let tx = self.commands.clone();
                self.job(true, move |flag| {
                    let client = orange_media::net::new_client().map_err(|e| e.to_string())?;
                    let found = network_wait(
                        orange_media::net::search_stations(
                            &client,
                            orange_media::radio::RADIO_BROWSER_API_BASE,
                            &query,
                            50,
                        ),
                        flag,
                    )?;
                    let stations = found
                        .into_iter()
                        .filter_map(|station| {
                            orange_media::radio::validate_custom_stream(
                                &station.name,
                                if station.url_resolved.is_empty() {
                                    &station.url
                                } else {
                                    &station.url_resolved
                                },
                            )
                        })
                        .map(|s| Station {
                            name: s.name,
                            url: s.url,
                        })
                        .collect();
                    tx.send(Action::RadioResults(stations))
                        .map_err(|e| e.to_string())?;
                    Ok("Station search complete".into())
                });
                save = false;
            }
            #[cfg(feature = "online")]
            Action::FetchLyrics => {
                let Some(track) = self.player.current().cloned() else {
                    self.fail("Play a track before looking up lyrics");
                    return;
                };
                let tx = self.commands.clone();
                self.job(true, move |flag| {
                    let client = orange_media::net::new_client().map_err(|e| e.to_string())?;
                    let hit = network_wait(
                        orange_media::net::fetch_lyrics(
                            &client,
                            &track.artist,
                            &track.title,
                            &track.album,
                            track.length_secs(),
                        ),
                        flag,
                    )?;
                    let lyrics = if hit.instrumental {
                        "Instrumental track".into()
                    } else {
                        hit.plain_lyrics
                            .or(hit.synced_lyrics)
                            .filter(|text| !text.is_empty())
                            .ok_or("No lyrics found for this track")?
                    };
                    tx.send(Action::LyricsResult(track.url, lyrics))
                        .map_err(|e| e.to_string())?;
                    Ok("Lyrics loaded".into())
                });
                save = false;
            }
            #[cfg(feature = "online")]
            Action::RadioResults(results) => {
                self.snapshot.radio_results = results;
                save = false;
            }
            #[cfg(feature = "online")]
            Action::LyricsResult(url, lyrics) => {
                self.snapshot.lyrics = Some((url, lyrics));
                save = false;
            }
            Action::ReportError(error) => {
                self.fail(error);
                save = false;
            }
            Action::Rate(url, rating) => {
                let result = orange_db::open_collection(
                    &self.collection.db_path,
                    orange_db::OpenMode::ReadWrite,
                )
                .and_then(|db| orange_db::library::set_rating(&db, &url, rating));
                if let Err(error) = result {
                    self.fail(error.to_string());
                } else {
                    self.refresh();
                }
                save = false;
            }
            Action::DismissError => {
                self.snapshot.error = None;
                save = false;
            }
            Action::Shutdown => self.shutdown = true,
        }
        self.sync_audio();
        if save {
            self.settings_pending = Some(Instant::now());
        }
    }
    fn restart(&mut self) {
        self.snapshot.position = 0;
        #[cfg(feature = "gst")]
        self.engine_url.clear();
    }
    fn advance(&mut self, manual: bool) {
        let Some(cursor) = self.player.cursor() else {
            return;
        };
        if !manual && self.player.stop_behaviour() == StopBehaviour::StopAfterCurrent {
            self.player.track_ended();
            return;
        }
        let next = next_index(
            self.player.queue(),
            cursor,
            self.snapshot.settings.repeat,
            self.snapshot.settings.shuffle,
            self.seed,
            manual,
        );
        if let Some(index) = next {
            self.player.play_at(index);
            self.snapshot.position = 0;
            #[cfg(feature = "gst")]
            self.engine_url.clear();
        } else {
            self.player.stop();
        }
    }
    fn seek(&mut self, seconds: u64) {
        #[cfg(feature = "gst")]
        if let Some(engine) = &self.engine {
            if let Err(error) = engine.seek_secs(seconds) {
                self.fail(error.to_string());
                return;
            }
        }
        self.snapshot.position = seconds;
    }
    fn sync_audio(&mut self) {
        #[cfg(feature = "gst")]
        {
            use orange_media::{
                audio_fx::NormalizationMode,
                backend::{AudioSink, FxChain, PlaybackChain},
                backend_gst::GstEngine,
            };
            let url = self
                .player
                .current()
                .map(|t| t.url.clone())
                .unwrap_or_default();
            if !self.player.state().is_active() {
                self.snapshot.duration = 0;
                if let Some(engine) = self.engine.take() {
                    if let Err(e) = engine.stop() {
                        self.fail(e.to_string());
                    }
                }
                self.engine_url.clear();
                return;
            }
            if self.engine_url != url || self.engine.is_none() {
                if let Some(engine) = self.engine.take() {
                    if let Err(e) = engine.stop() {
                        self.fail(e.to_string());
                    }
                }
                let sink = if std::env::var("ORANGE_AUDIO_OUTPUT").as_deref() == Ok("null") {
                    AudioSink::Null
                } else {
                    AudioSink::Auto
                };
                let eq = self.snapshot.settings.equalizer;
                let chain = PlaybackChain {
                    uri: url.clone(),
                    sink,
                    fx: FxChain {
                        equalizer_db: Some(eq),
                        spectrum_bands: Some(32),
                        ..FxChain::default()
                    },
                };
                match GstEngine::new_playback(
                    &chain,
                    Some(f64::from(self.player.volume()) / 100.0),
                    false,
                    NormalizationMode::Off,
                ) {
                    Ok(engine) => {
                        self.engine_url = url;
                        self.engine = Some(engine);
                    }
                    Err(e) => {
                        self.player.stop();
                        self.fail(e.to_string());
                        return;
                    }
                }
            }
            if let Some(engine) = &self.engine {
                let result = if self.player.state() == EngineState::Playing {
                    engine.play()
                } else {
                    engine.pause()
                };
                if let Err(e) = result {
                    self.player.stop();
                    self.fail(e.to_string());
                }
            }
        }
    }
    fn tick(&mut self) {
        if self
            .settings_pending
            .is_some_and(|time| time.elapsed() >= Duration::from_millis(300))
        {
            self.settings_pending = None;
            self.persist();
        }
        #[cfg(feature = "notify")]
        if self.player.state() == EngineState::Playing {
            if let Some(track) = self
                .player
                .current()
                .filter(|track| self.notified.as_deref() != Some(&track.url))
            {
                self.notified = Some(track.url.clone());
                let track = track.clone();
                if let Err(error) = std::thread::Builder::new()
                    .name("orange-notification".into())
                    .spawn(move || {
                        if !crate::notify::notify_track(&track.title, &track.artist, &track.album) {
                            tracing::debug!("Native notification service unavailable");
                        }
                    })
                {
                    tracing::warn!("Notification task failed: {error}");
                }
            }
        }

        #[cfg(all(feature = "dbus", target_os = "linux"))]
        {
            let pending = self.remotes.try_iter().collect::<Vec<_>>();
            for command in pending {
                use orange_media::mpris_server::MprisCommand as M;
                let action = match command {
                    M::PlayPause => Some(Action::PlayPause),
                    M::Play if self.player.state() != EngineState::Playing => {
                        Some(Action::PlayPause)
                    }
                    M::Pause if self.player.state() == EngineState::Playing => {
                        Some(Action::PlayPause)
                    }
                    M::Stop => Some(Action::Stop),
                    M::StopAfterCurrent => Some(Action::StopAfterCurrent),
                    M::Next => Some(Action::Next),
                    M::Previous => Some(Action::Previous),
                    M::Quit => Some(Action::Shutdown),
                    M::SetShuffle(value) => Some(Action::Shuffle(u8::from(value))),
                    M::SetLoop(value) => Some(Action::Repeat(match value.as_str() {
                        "Track" => 2,
                        "Playlist" => 1,
                        _ => 0,
                    })),
                    M::SetVolume(v) => Some(Action::Volume((v.clamp(0.0, 1.0) * 100.0) as u8)),
                    M::SeekMicros(delta) => Some(Action::Seek(
                        (self.snapshot.position as i64 + delta / 1_000_000).max(0) as u64,
                    )),
                    M::OpenUri(url) => Some(Action::Play(
                        vec![Song {
                            title: orange_core::paths::url_file_stem(&url),
                            url,
                            ..Song::default()
                        }],
                        0,
                    )),
                    _ => None,
                };
                if let Some(action) = action {
                    self.apply(action);
                }
            }
            self.mpris.sync_modes(
                self.snapshot.settings.repeat,
                self.snapshot.settings.shuffle,
            );
            self.mpris.sync_player(
                &self.player,
                (self.snapshot.position * 1_000_000) as i64,
                f64::from(self.player.volume()) / 100.0,
            );
        }
        #[cfg(feature = "gst")]
        {
            use orange_media::backend_gst::EngineEvent;
            let mut events = vec![];
            if let Some(engine) = &self.engine {
                let position = engine.position_nanos().unwrap_or(0) / 1_000_000_000;
                if self.snapshot.position != position {
                    self.snapshot.position = position;
                    self.dirty = true;
                }
                self.snapshot.duration = engine.duration_nanos().unwrap_or(0) / 1_000_000_000;
                for _ in 0..16 {
                    if let Some(event) = engine.poll_event(Duration::ZERO) {
                        events.push(event);
                    } else {
                        break;
                    }
                }
            }
            for event in events {
                match event {
                    EngineEvent::Eos => {
                        if let Some(track) = self.player.current() {
                            let now = SystemTime::now()
                                .duration_since(UNIX_EPOCH)
                                .map(|d| d.as_secs() as i64)
                                .unwrap_or(0);
                            let result = orange_db::open_collection(
                                &self.collection.db_path,
                                orange_db::OpenMode::ReadWrite,
                            )
                            .and_then(|db| orange_db::library::record_play(&db, &track.url, now));
                            if let Err(error) = result {
                                self.fail(error.to_string());
                            } else {
                                self.refresh();
                            }
                        }
                        self.advance(false);
                        self.sync_audio();
                        self.persist();
                        self.dirty = true;
                    }
                    EngineEvent::Error(e) => {
                        self.player.stop();
                        self.sync_audio();
                        self.fail(e);
                    }
                    EngineEvent::SpectrumTick { magnitudes_db } => {
                        self.snapshot.spectrum = magnitudes_db;
                        self.dirty = true;
                    }
                    _ => {}
                }
            }
        }
    }
    fn publish(&mut self) {
        if !self.dirty {
            return;
        }
        self.refresh_queue();
        self.snapshot.cursor = self.player.cursor();
        self.snapshot.playback = self.player.state();
        self.snapshot.shutdown = self.shutdown;
        self.snapshot.can_undo = !self.undo.is_empty();
        self.snapshot.can_redo = !self.redo.is_empty();
        if let Ok(mut update) = self.updates.lock() {
            *update = Some(self.snapshot.clone());
        }
        self.dirty = false;
    }
}

fn queue_songs(player: &Player) -> Vec<Song> {
    player
        .queue()
        .iter()
        .map(|t| Song {
            title: t.title.clone(),
            artist: t.artist.clone(),
            album: t.album.clone(),
            genre: t.genre.clone(),
            url: t.url.clone(),
            length_ns: t.length_ns,
            year: t.year,
            track: t.track,
            ..Song::default()
        })
        .collect()
}

pub fn next_index(
    queue: &[QueuedTrack],
    cursor: usize,
    repeat: u8,
    shuffle: u8,
    seed: u64,
    manual: bool,
) -> Option<usize> {
    let current = queue.get(cursor)?;
    if repeat == 2 && !manual {
        return Some(cursor);
    }
    let mut order = if shuffle == 1 {
        orange_playlist::model::shuffled_order(queue.len(), seed)
    } else {
        (0..queue.len()).collect::<Vec<_>>()
    };
    if repeat == 3 || shuffle == 2 {
        order.retain(|i| queue[*i].album == current.album && queue[*i].artist == current.artist);
        if shuffle == 2 {
            let positions = orange_playlist::model::shuffled_order(order.len(), seed);
            order = positions.into_iter().map(|i| order[i]).collect();
        }
    }
    let position = order.iter().position(|i| *i == cursor)?;
    order.get(position + 1).copied().or_else(|| {
        if repeat == 1 || repeat == 3 {
            order.first().copied()
        } else {
            None
        }
    })
}
fn songs_from_uris(uris: Vec<String>) -> Result<Vec<Song>, String> {
    let mut songs = Vec::new();
    for url in uris {
        if let Some(path) = orange_core::paths::file_url_to_path(&url) {
            if matches!(
                path.extension()
                    .and_then(|extension| extension.to_str())
                    .unwrap_or("")
                    .to_ascii_lowercase()
                    .as_str(),
                "m3u" | "m3u8" | "pls" | "xspf"
            ) {
                songs.extend(read_playlist(&path)?);
                continue;
            }
        }
        songs.push(Song {
            title: orange_core::paths::url_file_stem(&url),
            url,
            ..Song::default()
        });
    }
    Ok(songs)
}

fn read_playlist(path: &Path) -> Result<Vec<Song>, String> {
    use std::io::Read;
    const LIMIT: u64 = 8 * 1024 * 1024;
    let mut bytes = Vec::new();
    std::fs::File::open(path)
        .map_err(|e| e.to_string())?
        .take(LIMIT + 1)
        .read_to_end(&mut bytes)
        .map_err(|e| e.to_string())?;
    if bytes.len() as u64 > LIMIT {
        return Err("Playlist exceeds the 8 MB size limit.".into());
    }
    let text = String::from_utf8(bytes).map_err(|e| e.to_string())?;
    let entries = match path
        .extension()
        .and_then(|e| e.to_str())
        .unwrap_or("")
        .to_ascii_lowercase()
        .as_str()
    {
        "m3u" | "m3u8" => orange_playlist::parsers::parse_m3u(&text),
        "pls" => orange_playlist::parsers::parse_pls(&text),
        "xspf" => orange_playlist::parsers::parse_xspf_checked(&text)?,
        _ => return Err("Choose an M3U, PLS or XSPF playlist.".into()),
    };
    Ok(entries
        .into_iter()
        .map(|entry| {
            let url = if entry.url.contains("://") {
                entry.url
            } else {
                orange_core::paths::path_to_file_url(
                    &path.parent().unwrap_or(Path::new("")).join(entry.url),
                )
            };
            Song {
                title: if entry.title.trim().is_empty() {
                    orange_core::paths::url_file_stem(&url)
                } else {
                    entry.title
                },
                url,
                length_ns: entry.length_secs.unwrap_or(0).saturating_mul(1_000_000_000),
                ..Song::default()
            }
        })
        .collect())
}
fn write_new(path: &Path, data: &[u8]) -> Result<(), String> {
    use std::io::Write;
    let parent = path
        .parent()
        .filter(|p| !p.as_os_str().is_empty())
        .unwrap_or(Path::new("."));
    let mut file = tempfile::NamedTempFile::new_in(parent).map_err(|e| e.to_string())?;
    file.write_all(data).map_err(|e| e.to_string())?;
    file.as_file().sync_all().map_err(|e| e.to_string())?;
    file.persist_noclobber(path)
        .map_err(|e| format!("Cannot create export (existing files are preserved): {e}"))?;
    Ok(())
}

#[cfg(feature = "online")]
fn network_wait<T>(
    future: impl std::future::Future<Output = Result<T, orange_media::net::NetError>>,
    cancel: Arc<AtomicBool>,
) -> Result<T, String> {
    tokio::runtime::Builder::new_current_thread().enable_all().build().map_err(|e|e.to_string())?.block_on(async{
        tokio::select!{result=future=>result.map_err(|e|e.to_string()),_=async{while !cancel.load(Ordering::Relaxed){tokio::time::sleep(Duration::from_millis(50)).await;}}=>Err("Network operation cancelled".into())}
    })
}

#[cfg(test)]
mod tests {
    use super::*;
    #[test]
    fn launch_expands_local_playlists_in_order_and_preserves_remote_streams() {
        let root = tempfile::tempdir().unwrap();
        let m3u = root.path().join("Música 日本.M3U8");
        let pls = root.path().join("radio.pls");
        let xspf = root.path().join("tracks.xspf");
        std::fs::write(
            &m3u,
            "#EXTM3U\r\n日本.wav\r\n#EXTINF:3,Named track\r\nsecond.wav\r\n",
        )
        .unwrap();
        std::fs::write(
            &pls,
            "[playlist]\nFile1=https://example.org/radio\nTitle1=Radio\nNumberOfEntries=1\n",
        )
        .unwrap();
        std::fs::write(&xspf, "<playlist xmlns=\"http://xspf.org/ns/0/\"><trackList><track><location>last.flac</location><duration>4000</duration></track></trackList></playlist>").unwrap();
        let literal = orange_core::paths::path_to_file_url(&root.path().join("literal.wav"));
        let remote = "https://example.org/live.m3u8";
        let songs = songs_from_uris(vec![
            literal.clone(),
            orange_core::paths::path_to_file_url(&m3u),
            orange_core::paths::path_to_file_url(&pls),
            orange_core::paths::path_to_file_url(&xspf),
            remote.into(),
        ])
        .unwrap();
        assert_eq!(songs.len(), 6);
        assert_eq!(songs[0].url, literal);
        assert_eq!(
            songs[1].url,
            orange_core::paths::path_to_file_url(&root.path().join("日本.wav"))
        );
        assert_eq!(songs[1].title, "日本");
        assert_eq!(songs[2].title, "Named track");
        assert_eq!(songs[2].length_ns, 3_000_000_000);
        assert_eq!(songs[3].title, "Radio");
        assert_eq!(songs[4].title, "last");
        assert_eq!(songs[4].length_ns, 4_000_000_000);
        assert_eq!(songs[5].url, remote);
    }
    #[test]
    fn launch_rejects_invalid_playlist_before_returning_a_partial_queue() {
        let root = tempfile::tempdir().unwrap();
        let broken = root.path().join("broken.xspf");
        std::fs::write(&broken, "<playlist><trackList>").unwrap();
        let result = songs_from_uris(vec![
            "https://example.org/radio".into(),
            orange_core::paths::path_to_file_url(&broken),
        ]);
        assert!(result.unwrap_err().contains("Invalid XSPF"));
    }
    #[test]
    fn export_commits_complete_file_without_overwriting_existing_data() {
        let root = tempfile::tempdir().unwrap();
        let path = root.path().join("Música 日本.m3u");
        write_new(&path, b"#EXTM3U\nfirst.wav\n").unwrap();
        assert!(write_new(&path, b"replacement").is_err());
        assert_eq!(std::fs::read(&path).unwrap(), b"#EXTM3U\nfirst.wav\n");
        assert_eq!(std::fs::read_dir(root.path()).unwrap().count(), 1);
    }
    fn queue() -> Vec<QueuedTrack> {
        (0..4)
            .map(|i| QueuedTrack {
                title: i.to_string(),
                album: if i < 2 { "A" } else { "B" }.into(),
                ..QueuedTrack::default()
            })
            .collect()
    }
    #[test]
    fn repeat_track_does_not_trap_manual_next() {
        let q = queue();
        assert_eq!(next_index(&q, 0, 2, 0, 1, false), Some(0));
        assert_eq!(next_index(&q, 0, 2, 0, 1, true), Some(1));
    }
    #[test]
    fn repeat_album_wraps_only_that_album() {
        let q = queue();
        assert_eq!(next_index(&q, 1, 3, 0, 1, false), Some(0));
        assert_eq!(next_index(&q, 3, 3, 0, 1, false), Some(2));
    }
    #[test]
    fn shuffle_uses_the_actual_order_and_wraps() {
        let q = queue();
        let order = orange_playlist::model::shuffled_order(q.len(), 7);
        for pair in order.windows(2) {
            assert_eq!(next_index(&q, pair[0], 0, 1, 7, false), Some(pair[1]));
        }
        assert_eq!(
            next_index(&q, *order.last().unwrap(), 1, 1, 7, false),
            Some(order[0])
        );
    }
}
