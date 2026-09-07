//! Playback state machine and queue logic.
//! Mirrors `Player`/`EngineBase` states: the GStreamer backend (behind the
//! `gst` feature) reports pipeline changes into this machine, which owns
//! what the UI, MPRIS, and scrobblers observe. Bit-perfect by default: no
//! resampling or volume scaling touches the stream unless the user enables
//! the equalizer, normalization, or software volume.

/// Engine state, mirroring `Engine::State`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum EngineState {
    #[default]
    Empty,
    Idle,
    Playing,
    Paused,
}

impl EngineState {
    pub fn is_active(self) -> bool {
        matches!(self, Self::Playing | Self::Paused)
    }
}

/// What to do when the current track ends, mirroring stop-after-track.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum StopBehaviour {
    #[default]
    Continue,
    StopAfterCurrent,
}

/// One playlist/queue row, with the columns the playlist table shows.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct QueuedTrack {
    pub url: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub genre: String,
    pub length_ns: i64,
    pub year: i64,
    pub track: i64,
}

impl QueuedTrack {
    pub fn from_song(song: &orange_core::song::Song) -> Self {
        Self {
            url: song.url.clone(),
            title: song.display_title(),
            artist: song.display_artist().to_string(),
            album: song.display_album().to_string(),
            genre: song.genre.clone(),
            length_ns: song.length_ns,
            year: song.year,
            track: song.track,
        }
    }

    pub fn length_secs(&self) -> i64 {
        self.length_ns / 1_000_000_000
    }

    pub fn format_length(&self) -> String {
        orange_core::song::format_duration_secs(self.length_secs())
    }
}

/// The player: queue + cursor + state. UI-agnostic.
#[derive(Debug, Default)]
pub struct Player {
    queue: Vec<QueuedTrack>,
    cursor: Option<usize>,
    state: EngineState,
    stop_behaviour: StopBehaviour,
    volume_percent: u8,
}

impl Player {
    pub fn new() -> Self {
        Self {
            volume_percent: 100,
            ..Self::default()
        }
    }

    pub fn state(&self) -> EngineState {
        self.state
    }

    pub fn current(&self) -> Option<&QueuedTrack> {
        self.cursor.and_then(|i| self.queue.get(i))
    }

    pub fn enqueue(&mut self, track: QueuedTrack) {
        self.queue.push(track);
    }

    pub fn enqueue_many(&mut self, tracks: impl IntoIterator<Item = QueuedTrack>) {
        self.queue.extend(tracks);
    }

    pub fn queue(&self) -> &[QueuedTrack] {
        &self.queue
    }

    pub fn cursor(&self) -> Option<usize> {
        self.cursor
    }

    pub fn queue_len(&self) -> usize {
        self.queue.len()
    }

    pub fn clear_queue(&mut self) {
        self.queue.clear();
        self.cursor = None;
        self.state = EngineState::Empty;
    }

    pub fn remove_at(&mut self, index: usize) -> Option<QueuedTrack> {
        if index >= self.queue.len() {
            return None;
        }
        let removed = self.queue.remove(index);
        self.cursor = match self.cursor {
            Some(cursor) if cursor == index => {
                if self.queue.is_empty() {
                    self.state = EngineState::Empty;
                    None
                } else {
                    let next = index.min(self.queue.len() - 1);
                    Some(next)
                }
            }
            Some(cursor) if cursor > index => Some(cursor - 1),
            other => other,
        };
        Some(removed)
    }

    /// Replace the queue and start playing at `start`. Empty input stops.
    pub fn replace_and_play(&mut self, tracks: Vec<QueuedTrack>, start: usize) -> bool {
        self.queue = tracks;
        if self.queue.is_empty() {
            self.cursor = None;
            self.state = EngineState::Empty;
            return false;
        }
        self.play_at(start.min(self.queue.len() - 1))
    }

    pub fn play_at(&mut self, index: usize) -> bool {
        if index >= self.queue.len() {
            return false;
        }
        self.cursor = Some(index);
        self.state = EngineState::Playing;
        true
    }

    pub fn toggle_play_pause(&mut self) {
        match self.state {
            EngineState::Playing => self.state = EngineState::Paused,
            EngineState::Paused => self.state = EngineState::Playing,
            EngineState::Empty | EngineState::Idle => {
                if !self.queue.is_empty() {
                    if self.cursor.is_none() {
                        self.cursor = Some(0);
                    }
                    self.state = EngineState::Playing;
                }
            }
        }
    }

    pub fn stop(&mut self) {
        self.state = EngineState::Idle;
    }

    /// Manual skip forward. Unlike [`Self::track_ended`], never consumes an
    /// armed stop-after-current. Returns false at the end of the queue.
    /// Named after the transport button, not [`Iterator`]: the return is a
    /// success flag (not `Option<Item>`) and skipping is bidirectional.
    #[allow(clippy::should_implement_trait)]
    pub fn next(&mut self) -> bool {
        let Some(cursor) = self.cursor else {
            return false;
        };
        self.play_at(cursor + 1)
    }

    /// Manual skip back. Returns false on the first track or with no cursor.
    pub fn previous(&mut self) -> bool {
        match self.cursor {
            Some(cursor) if cursor > 0 => self.play_at(cursor - 1),
            _ => false,
        }
    }

    /// Advance at natural track end. Returns the new cursor, or None when
    /// playback stops (end of queue, or stop-after-current armed).
    pub fn track_ended(&mut self) -> Option<usize> {
        if self.stop_behaviour == StopBehaviour::StopAfterCurrent {
            self.stop_behaviour = StopBehaviour::Continue;
            self.state = EngineState::Idle;
            return None;
        }
        let next = self.cursor? + 1;
        if next < self.queue.len() {
            self.cursor = Some(next);
            Some(next)
        } else {
            self.state = EngineState::Empty;
            self.cursor = None;
            None
        }
    }

    pub fn set_stop_after_current(&mut self) {
        self.stop_behaviour = StopBehaviour::Continue;
        if self.state == EngineState::Playing {
            self.stop_behaviour = StopBehaviour::StopAfterCurrent;
        }
    }

    /// Software volume 0-100. Bit-perfect path keeps this at 100 and drives
    /// the hardware mixer instead; the engine only scales when asked.
    pub fn set_volume(&mut self, percent: u8) {
        self.volume_percent = percent.min(100);
    }

    pub fn volume(&self) -> u8 {
        self.volume_percent
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn track(name: &str) -> QueuedTrack {
        QueuedTrack {
            url: format!("file:///m/{name}"),
            title: name.into(),
            ..QueuedTrack::default()
        }
    }

    #[test]
    fn play_pause_stop_cycle() {
        let mut player = Player::new();
        player.enqueue(track("a"));
        assert_eq!(player.state(), EngineState::Empty);
        player.toggle_play_pause();
        assert_eq!(player.state(), EngineState::Playing);
        assert_eq!(player.current().unwrap().title, "a");
        player.toggle_play_pause();
        assert_eq!(player.state(), EngineState::Paused);
        player.stop();
        assert_eq!(player.state(), EngineState::Idle);
    }

    #[test]
    fn play_after_stop_resumes_cursor() {
        let mut player = Player::new();
        player.enqueue(track("a"));
        player.enqueue(track("b"));
        assert!(player.play_at(1));
        player.stop();
        player.toggle_play_pause();
        assert_eq!(player.state(), EngineState::Playing);
        assert_eq!(player.current().unwrap().title, "b");
    }

    #[test]
    fn queue_runs_to_empty() {
        let mut player = Player::new();
        player.enqueue(track("a"));
        player.enqueue(track("b"));
        assert!(player.play_at(0));
        assert_eq!(player.track_ended(), Some(1));
        assert_eq!(player.track_ended(), None);
        assert_eq!(player.state(), EngineState::Empty);
        assert!(!player.play_at(7));
    }

    #[test]
    fn stop_after_current_arms_once() {
        let mut player = Player::new();
        player.enqueue(track("a"));
        player.enqueue(track("b"));
        assert!(player.play_at(0));
        player.set_stop_after_current();
        assert_eq!(player.track_ended(), None);
        assert_eq!(player.state(), EngineState::Idle);
        // Disarmed: a second end-of-track continues normally.
        assert!(player.play_at(0));
        assert_eq!(player.track_ended(), Some(1));
    }

    #[test]
    fn volume_clamps() {
        let mut player = Player::new();
        player.set_volume(250);
        assert_eq!(player.volume(), 100);
    }

    #[test]
    fn manual_skip() {
        let mut player = Player::new();
        assert!(!player.next());
        assert!(!player.previous());
        player.enqueue(track("a"));
        player.enqueue(track("b"));
        assert!(player.play_at(0));
        assert!(player.next());
        assert_eq!(player.current().unwrap().title, "b");
        // End of queue: stay put, keep playing.
        assert!(!player.next());
        assert_eq!(player.current().unwrap().title, "b");
        assert!(player.previous());
        assert_eq!(player.current().unwrap().title, "a");
        assert!(!player.previous());
    }

    #[test]
    fn skip_keeps_stop_after_current_armed() {
        let mut player = Player::new();
        player.enqueue(track("a"));
        player.enqueue(track("b"));
        assert!(player.play_at(0));
        player.set_stop_after_current();
        assert!(player.next());
        // Still armed: the natural end of track "b" stops.
        assert_eq!(player.track_ended(), None);
        assert_eq!(player.state(), EngineState::Idle);
    }

    #[test]
    fn replace_and_clear_queue() {
        let mut player = Player::new();
        assert!(player.replace_and_play(vec![track("a"), track("b")], 1));
        assert_eq!(player.current().unwrap().title, "b");
        assert_eq!(player.queue_len(), 2);
        player.clear_queue();
        assert_eq!(player.queue_len(), 0);
        assert_eq!(player.state(), EngineState::Empty);
    }
}
