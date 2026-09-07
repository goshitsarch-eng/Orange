//! Headless MPRIS host (feature `dbus`): a real [`Player`] published on the
//! session bus, optionally driving a live [`GstEngine`] (feature `gst`) and
//! native notifications (feature `notify`).
//!
//! `orange --serve [FILES...]` runs this: files enqueue, the first starts,
//! remotes control it over MPRIS, and `--quit`-style desktop actions work.
//! Without audio hardware the engine fails to start and the host keeps
//! serving state only, saying so on stderr.

use std::sync::mpsc;
use std::time::Duration;

use orange_media::mpris::{MprisMetadata, BUS_NAME};
use orange_media::mpris_server::{serve, MprisCommand, MprisState, SharedState};
use orange_media::playback::{EngineState, Player, QueuedTrack};

/// Apply one remote command. Returns true when the shell should quit.
pub fn apply_command(player: &mut Player, command: &MprisCommand) -> bool {
    match command {
        MprisCommand::Play => {
            if !matches!(player.state(), EngineState::Playing) {
                player.toggle_play_pause();
            }
        }
        MprisCommand::Pause => {
            if matches!(player.state(), EngineState::Playing) {
                player.toggle_play_pause();
            }
        }
        MprisCommand::PlayPause => player.toggle_play_pause(),
        MprisCommand::Stop => player.stop(),
        MprisCommand::StopAfterCurrent => player.set_stop_after_current(),
        MprisCommand::Next => {
            player.next();
        }
        MprisCommand::Previous => {
            player.previous();
        }
        MprisCommand::SeekMicros(_) => {
            // Applied to the live engine by the serve loop, if attached.
        }
        MprisCommand::OpenUri(uri) => {
            let title = uri.rsplit('/').next().unwrap_or(uri).to_string();
            player.enqueue(QueuedTrack {
                url: uri.clone(),
                title,
                ..QueuedTrack::default()
            });
            player.play_at(player.queue_len().saturating_sub(1));
        }
        MprisCommand::SetVolume(volume) => {
            player.set_volume((volume.clamp(0.0, 1.0) * 100.0) as u8);
        }
        // Sequencing lives in the UI shell's playlist; the daemon keeps a
        // simple queue, so these only persist in published MPRIS state.
        MprisCommand::SetShuffle(_) | MprisCommand::SetLoop(_) => {}
        MprisCommand::Quit => return true,
    }
    false
}

/// MPRIS-facing host state: published snapshot plus change tracking.
pub struct MprisHost {
    /// Snapshot published on the bus. Cloned into the server task.
    pub shared: SharedState,
    /// Last published track URL. `None` until the first sync so the first
    /// publish always counts as a change (never collide with the empty
    /// queue, whose URL is `""`).
    last_url: Option<String>,
    generation: u64,
}

impl MprisHost {
    pub fn new() -> Self {
        Self {
            shared: SharedState::new(MprisState::default()),
            last_url: None,
            generation: 0,
        }
    }

    /// Publish `player` into shared state. Returns true on track change
    /// (callers notify and reattach the engine then).
    pub fn sync_player(&mut self, player: &Player, position_micros: i64, volume01: f64) -> bool {
        let current = player.current();
        let url = current.map(|track| track.url.as_str()).unwrap_or("");
        let changed = self.last_url.as_deref() != Some(url);
        if changed {
            self.last_url = Some(url.to_string());
            self.generation += 1;
        }
        let metadata = current.map(|track| {
            MprisMetadata::from_song(&track.title, "", "", 0, self.generation as usize, "")
        });
        let status = match player.state() {
            EngineState::Playing => orange_media::mpris::PlaybackStatus::Playing,
            EngineState::Paused => orange_media::mpris::PlaybackStatus::Paused,
            EngineState::Empty | EngineState::Idle => orange_media::mpris::PlaybackStatus::Stopped,
        };
        self.shared.update(|state| {
            state.status = status;
            state.metadata = metadata;
            state.position_micros = position_micros;
            state.volume = volume01.clamp(0.0, 1.0);
            state.can_go_next = player.current().is_some();
            state.can_go_previous = player.current().is_some();
        });
        changed
    }

    /// Spawn the MPRIS server on its own thread + runtime, forwarding to
    /// `tx`. The thread serves until the process exits.
    pub fn spawn_server(
        &self,
        bus_name: String,
        tx: mpsc::Sender<MprisCommand>,
    ) -> std::thread::JoinHandle<()> {
        spawn_server(bus_name, self.shared.clone(), tx)
    }
}

/// Spawn the MPRIS server on its own thread + runtime (see
/// [`MprisHost::spawn_server`]).
pub fn spawn_server(
    bus_name: String,
    shared: SharedState,
    tx: mpsc::Sender<MprisCommand>,
) -> std::thread::JoinHandle<()> {
    std::thread::spawn(move || {
        let Ok(runtime) = tokio::runtime::Builder::new_current_thread()
            .enable_all()
            .build()
        else {
            eprintln!("orange: mpris runtime failed");
            return;
        };
        if let Err(e) = runtime.block_on(serve(bus_name, shared, tx)) {
            eprintln!("orange: mpris serve ended: {e}");
        }
    })
}

impl Default for MprisHost {
    fn default() -> Self {
        Self::new()
    }
}

/// Serve MPRIS until Quit (or EOF on the command channel). `initial_uris`
/// enqueue before serving; the first starts playing immediately.
/// Returns the process exit code.
pub fn serve_forever(initial_uris: Vec<String>) -> i32 {
    let mut player = Player::new();
    for uri in initial_uris {
        let title = uri.rsplit('/').next().unwrap_or(&uri).to_string();
        player.enqueue(QueuedTrack {
            url: uri,
            title,
            ..QueuedTrack::default()
        });
    }
    if player.queue_len() > 0 {
        player.play_at(0);
    }

    let mut host = MprisHost::new();
    host.sync_player(&player, 0, 1.0);
    let (tx, rx) = mpsc::channel::<MprisCommand>();
    let _server = host.spawn_server(BUS_NAME.to_string(), tx);
    eprintln!("orange: serving MPRIS as {BUS_NAME} (Ctrl-C or Quit to stop)");

    #[cfg(feature = "gst")]
    let mut engine: Option<orange_media::backend_gst::GstEngine> = None;
    #[cfg(feature = "gst")]
    let mut engine_url = String::new();

    loop {
        // Engine events first: natural track end advances the queue; the
        // reattach block below rebuilds audio for the new current track.
        #[cfg(feature = "gst")]
        if let Some(engine) = engine.as_ref() {
            match engine.poll_event(Duration::from_millis(0)) {
                Some(orange_media::backend_gst::EngineEvent::Eos) => {
                    player.track_ended();
                }
                Some(orange_media::backend_gst::EngineEvent::Error(e)) => {
                    eprintln!("orange: engine error: {e}");
                }
                _ => {}
            }
        }

        match rx.recv_timeout(Duration::from_millis(100)) {
            Ok(MprisCommand::Quit) => break,
            Ok(command) => {
                if apply_command(&mut player, &command) {
                    break;
                }
                #[cfg(feature = "gst")]
                handle_engine_command(&command, &player, &mut engine, &mut engine_url);
            }
            Err(mpsc::RecvTimeoutError::Timeout) => {}
            Err(mpsc::RecvTimeoutError::Disconnected) => break,
        }

        #[cfg(feature = "gst")]
        {
            // (Re)attach the engine when the current track needs audio.
            let want_url = player
                .current()
                .filter(|_| matches!(player.state(), EngineState::Playing))
                .map(|track| track.url.clone())
                .unwrap_or_default();
            if want_url != engine_url {
                if want_url.is_empty() {
                    if let Some(engine) = engine.take() {
                        engine.stop().ok();
                    }
                    engine_url.clear();
                } else {
                    attach_engine(&mut engine_url, &mut engine, &player);
                }
            }
            let position = engine
                .as_ref()
                .and_then(|engine| engine.position_nanos())
                .map(|nanos| (nanos / 1000) as i64)
                .unwrap_or(0);
            if host.sync_player(&player, position, player.volume() as f64 / 100.0) {
                notify_current(&player);
            }
        }
        #[cfg(not(feature = "gst"))]
        {
            if host.sync_player(&player, 0, player.volume() as f64 / 100.0) {
                notify_current(&player);
            }
        }
    }
    0
}

/// (Re)build the engine for the player's current track, best-effort:
/// audio-hardware failures fall back to state-only serving.
#[cfg(feature = "gst")]
fn attach_engine(
    engine_url: &mut String,
    engine: &mut Option<orange_media::backend_gst::GstEngine>,
    player: &Player,
) {
    use orange_media::backend::{AudioSink, FxChain, PlaybackChain};
    use orange_media::backend_gst::GstEngine;

    if let Some(current) = player.current() {
        let volume = player.volume();
        let software_volume = if volume >= 100 {
            None
        } else {
            Some(volume as f64 / 100.0)
        };
        let chain = PlaybackChain {
            uri: current.url.clone(),
            sink: AudioSink::Auto,
            fx: FxChain::default(),
        };
        match GstEngine::new_playback(
            &chain,
            software_volume,
            false,
            orange_media::audio_fx::NormalizationMode::Off,
        )
        .and_then(|engine| {
            engine.play()?;
            Ok(engine)
        }) {
            Ok(new_engine) => {
                *engine = Some(new_engine);
                *engine_url = current.url.clone();
            }
            Err(e) => {
                eprintln!("orange: audio engine unavailable ({e}); serving state only");
                *engine = None;
                engine_url.clear();
            }
        }
    }
}

/// Live engine reactions to remote commands that need more than state.
#[cfg(feature = "gst")]
fn handle_engine_command(
    command: &MprisCommand,
    player: &Player,
    engine: &mut Option<orange_media::backend_gst::GstEngine>,
    _engine_url: &mut String,
) {
    match command {
        MprisCommand::Pause | MprisCommand::Stop => {
            if let Some(engine) = engine.as_ref() {
                if matches!(command, MprisCommand::Pause) {
                    engine.pause().ok();
                } else {
                    engine.stop().ok();
                }
            }
        }
        MprisCommand::SeekMicros(micros) => {
            if let Some(engine) = engine.as_ref() {
                engine.seek_secs((*micros).max(0) as u64 / 1_000_000).ok();
            }
        }
        MprisCommand::Play | MprisCommand::PlayPause if player.state() == EngineState::Paused => {
            if let Some(engine) = engine.as_ref() {
                engine.play().ok();
            }
        }
        _ => {}
    }
    let _ = player;
}

#[cfg(feature = "notify")]
fn notify_current(player: &Player) {
    if let Some(track) = player.current() {
        crate::notify::notify_track(&track.title, "", "");
    }
}

#[cfg(not(feature = "notify"))]
fn notify_current(_player: &Player) {}

#[cfg(test)]
mod tests {
    use super::*;

    fn queued(title: &str) -> Player {
        let mut player = Player::new();
        player.enqueue(QueuedTrack {
            url: format!("file:///music/{title}.flac"),
            title: title.to_string(),
            ..QueuedTrack::default()
        });
        player
    }

    #[test]
    fn commands_drive_the_queue() {
        let mut player = queued("a");
        assert!(!apply_command(&mut player, &MprisCommand::PlayPause));
        assert_eq!(player.state(), EngineState::Playing);
        assert!(apply_command(&mut player, &MprisCommand::Quit));
        let mut player = queued("a");
        player.enqueue(QueuedTrack {
            url: "file:///music/b.flac".to_string(),
            title: "b".to_string(),
            ..QueuedTrack::default()
        });
        assert!(player.play_at(0));
        assert!(!apply_command(&mut player, &MprisCommand::Next));
        assert_eq!(player.current().unwrap().title, "b");
        assert!(!apply_command(&mut player, &MprisCommand::StopAfterCurrent));
        assert_eq!(player.track_ended(), None);
    }

    #[test]
    fn open_uri_enqueues_and_plays() {
        let mut player = Player::new();
        assert!(!apply_command(
            &mut player,
            &MprisCommand::OpenUri("file:///music/new.opus".to_string())
        ));
        assert_eq!(player.current().unwrap().title, "new.opus");
        assert_eq!(player.state(), EngineState::Playing);
    }

    #[test]
    fn sync_publishes_state_and_spots_track_changes() {
        let mut host = MprisHost::new();
        let mut player = queued("a");
        assert!(host.sync_player(&player, 0, 1.0));
        assert!(!host.sync_player(&player, 1000, 1.0));
        let state = host.shared.get();
        assert_eq!(state.status, orange_media::mpris::PlaybackStatus::Stopped);
        player.play_at(0);
        // No cursor was set before, so silence ("") -> "a" is a real track
        // change: the serve loop must notify and (re)attach the engine.
        assert!(host.sync_player(&player, 0, 0.5));
        let state = host.shared.get();
        assert_eq!(state.status, orange_media::mpris::PlaybackStatus::Playing);
        assert_eq!(state.volume, 0.5);
        let title = state.metadata.unwrap().title;
        assert_eq!(title, "a");
    }
}
