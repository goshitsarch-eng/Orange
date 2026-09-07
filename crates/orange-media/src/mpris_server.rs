//! MPRIS2 server (feature `dbus`).
//!
//! Exports `org.mpris.MediaPlayer2` + `org.mpris.MediaPlayer2.Player` on
//! [`crate::mpris::OBJECT_PATH`], driven by shared [`MprisState`]. Incoming
//! method calls become [`MprisCommand`]s on a std channel for the shell to
//! apply; setters update local state optimistically so reads stay coherent
//! between client round-trips. No `PropertiesChanged` signals yet: polling
//! clients (the common case for media keys and tray widgets) work fully.

use std::collections::HashMap;
use std::sync::{mpsc, Arc, Mutex};

use zbus::zvariant::{Array, ObjectPath, OwnedValue, Value};
use zbus::{interface, Connection};

use crate::mpris::{self, MprisMetadata, PlaybackStatus};

/// Command from a remote client to the shell.
#[derive(Debug, Clone, PartialEq)]
pub enum MprisCommand {
    Play,
    Pause,
    PlayPause,
    Stop,
    StopAfterCurrent,
    Next,
    Previous,
    SeekMicros(i64),
    OpenUri(String),
    SetVolume(f64),
    SetShuffle(bool),
    SetLoop(String),
    Quit,
}

/// Channel carrying [`MprisCommand`]s to the shell.
pub type CommandSender = mpsc::Sender<MprisCommand>;

/// Observable player state published on the bus.
#[derive(Debug, Clone)]
pub struct MprisState {
    pub status: PlaybackStatus,
    pub metadata: Option<MprisMetadata>,
    pub volume: f64,
    pub position_micros: i64,
    pub can_play: bool,
    pub can_pause: bool,
    pub can_go_next: bool,
    pub can_go_previous: bool,
    pub can_seek: bool,
    pub shuffle: bool,
    pub loop_status: String,
}

impl Default for MprisState {
    fn default() -> Self {
        Self {
            status: PlaybackStatus::Stopped,
            metadata: None,
            volume: 1.0,
            position_micros: 0,
            can_play: true,
            can_pause: true,
            can_go_next: true,
            can_go_previous: true,
            can_seek: true,
            shuffle: false,
            loop_status: "None".to_string(),
        }
    }
}

/// Shareable handle to [`MprisState`].
#[derive(Debug, Clone, Default)]
pub struct SharedState {
    inner: Arc<Mutex<MprisState>>,
}

impl SharedState {
    pub fn new(state: MprisState) -> Self {
        Self {
            inner: Arc::new(Mutex::new(state)),
        }
    }

    pub fn get(&self) -> MprisState {
        self.inner.lock().expect("mpris state").clone()
    }

    pub fn update(&self, f: impl FnOnce(&mut MprisState)) {
        f(&mut self.inner.lock().expect("mpris state"));
    }
}

fn owned_str(value: &str) -> OwnedValue {
    OwnedValue::try_from(Value::from(value.to_string())).expect("string to value")
}

fn owned_i64(value: i64) -> OwnedValue {
    OwnedValue::try_from(Value::from(value)).expect("i64 to value")
}

fn owned_trackid(trackid: &str) -> OwnedValue {
    match ObjectPath::try_from(trackid) {
        Ok(path) => OwnedValue::from(path),
        Err(_) => OwnedValue::from(
            ObjectPath::try_from("/org/mpris/MediaPlayer2/Track/0").expect("fallback trackid"),
        ),
    }
}

fn owned_str_array(items: &[String]) -> OwnedValue {
    let array = Array::from(items.to_vec());
    OwnedValue::try_from(Value::Array(array)).expect("string array to value")
}

/// MPRIS metadata dict (`a{sv}`) for one track.
pub fn metadata_dict(meta: &MprisMetadata) -> HashMap<String, OwnedValue> {
    let mut map = HashMap::new();
    map.insert("mpris:trackid".to_string(), owned_trackid(&meta.trackid));
    map.insert(
        "mpris:length".to_string(),
        owned_i64(meta.length_microseconds),
    );
    map.insert("xesam:title".to_string(), owned_str(&meta.title));
    map.insert("xesam:artist".to_string(), owned_str_array(&meta.artist));
    map.insert("xesam:album".to_string(), owned_str(&meta.album));
    if !meta.art_url.is_empty() {
        map.insert("mpris:artUrl".to_string(), owned_str(&meta.art_url));
    }
    map
}

struct RootIface {
    tx: CommandSender,
}

#[interface(name = "org.mpris.MediaPlayer2")]
impl RootIface {
    #[zbus(property)]
    fn identity(&self) -> String {
        mpris::IDENTITY.to_string()
    }

    #[zbus(property)]
    fn desktop_entry(&self) -> String {
        mpris::DESKTOP_ENTRY.to_string()
    }

    #[zbus(property)]
    fn supported_uri_schemes(&self) -> Vec<String> {
        vec![
            "file".to_string(),
            "http".to_string(),
            "https".to_string(),
            "cdda".to_string(),
        ]
    }

    #[zbus(property)]
    fn supported_mime_types(&self) -> Vec<String> {
        vec![
            "audio/mpeg".to_string(),
            "audio/flac".to_string(),
            "audio/ogg".to_string(),
            "audio/opus".to_string(),
            "audio/mp4".to_string(),
            "audio/x-wav".to_string(),
        ]
    }

    #[zbus(property)]
    fn can_quit(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn can_raise(&self) -> bool {
        false
    }

    #[zbus(property)]
    fn has_track_list(&self) -> bool {
        false
    }

    fn quit(&self) {
        let _ = self.tx.send(MprisCommand::Quit);
    }

    fn raise(&self) {}
}

struct PlayerIface {
    state: SharedState,
    tx: CommandSender,
}

#[interface(name = "org.mpris.MediaPlayer2.Player")]
impl PlayerIface {
    #[zbus(property)]
    fn playback_status(&self) -> String {
        self.state.get().status.as_str().to_string()
    }

    #[zbus(property)]
    fn metadata(&self) -> HashMap<String, OwnedValue> {
        match &self.state.get().metadata {
            Some(meta) => metadata_dict(meta),
            None => HashMap::new(),
        }
    }

    #[zbus(property)]
    fn volume(&self) -> f64 {
        self.state.get().volume
    }

    #[zbus(property)]
    fn set_volume(&self, volume: f64) {
        let volume = volume.clamp(0.0, 1.0);
        self.state.update(|state| state.volume = volume);
        let _ = self.tx.send(MprisCommand::SetVolume(volume));
    }

    #[zbus(property)]
    fn position(&self) -> i64 {
        self.state.get().position_micros
    }

    #[zbus(property)]
    fn rate(&self) -> f64 {
        1.0
    }

    #[zbus(property)]
    fn minimum_rate(&self) -> f64 {
        1.0
    }

    #[zbus(property)]
    fn maximum_rate(&self) -> f64 {
        1.0
    }

    #[zbus(property)]
    fn can_play(&self) -> bool {
        self.state.get().can_play
    }

    #[zbus(property)]
    fn can_pause(&self) -> bool {
        self.state.get().can_pause
    }

    #[zbus(property)]
    fn can_go_next(&self) -> bool {
        self.state.get().can_go_next
    }

    #[zbus(property)]
    fn can_go_previous(&self) -> bool {
        self.state.get().can_go_previous
    }

    #[zbus(property)]
    fn can_seek(&self) -> bool {
        self.state.get().can_seek
    }

    #[zbus(property)]
    fn can_control(&self) -> bool {
        true
    }

    #[zbus(property)]
    fn shuffle(&self) -> bool {
        self.state.get().shuffle
    }

    #[zbus(property)]
    fn set_shuffle(&self, shuffle: bool) {
        self.state.update(|state| state.shuffle = shuffle);
        let _ = self.tx.send(MprisCommand::SetShuffle(shuffle));
    }

    #[zbus(property)]
    fn loop_status(&self) -> String {
        self.state.get().loop_status.clone()
    }

    #[zbus(property)]
    fn set_loop_status(&self, status: String) {
        self.state
            .update(|state| state.loop_status = status.clone());
        let _ = self.tx.send(MprisCommand::SetLoop(status));
    }

    fn play(&self) {
        let _ = self.tx.send(MprisCommand::Play);
    }

    fn pause(&self) {
        let _ = self.tx.send(MprisCommand::Pause);
    }

    fn play_pause(&self) {
        let _ = self.tx.send(MprisCommand::PlayPause);
    }

    fn stop(&self) {
        let _ = self.tx.send(MprisCommand::Stop);
    }

    fn next(&self) {
        let _ = self.tx.send(MprisCommand::Next);
    }

    fn previous(&self) {
        let _ = self.tx.send(MprisCommand::Previous);
    }

    fn seek(&self, offset_micros: i64) {
        let _ = self.tx.send(MprisCommand::SeekMicros(offset_micros));
    }

    fn set_position(&self, _track_id: ObjectPath<'_>, position_micros: i64) {
        let _ = self.tx.send(MprisCommand::SeekMicros(position_micros));
    }

    fn open_uri(&self, uri: String) {
        let _ = self.tx.send(MprisCommand::OpenUri(uri));
    }
}

/// Orange extensions beyond MPRIS: queue controls the spec has no method
/// for (driven by the desktop `StopAfterCurrent` action).
struct OrangeIface {
    tx: CommandSender,
}

#[interface(name = "org.goshapps.Orange.Player")]
impl OrangeIface {
    fn stop_after_current(&self) {
        let _ = self.tx.send(MprisCommand::StopAfterCurrent);
    }
}

/// Serve MPRIS on `bus_name` until the future is cancelled. Owns the name;
/// returns when the connection drops or the task is aborted.
pub async fn serve(bus_name: String, state: SharedState, tx: CommandSender) -> zbus::Result<()> {
    let conn = Connection::session().await?;
    conn.request_name(bus_name).await?;
    conn.object_server()
        .at(mpris::OBJECT_PATH, RootIface { tx: tx.clone() })
        .await?;
    conn.object_server()
        .at(
            mpris::OBJECT_PATH,
            PlayerIface {
                state,
                tx: tx.clone(),
            },
        )
        .await?;
    conn.object_server()
        .at(mpris::OBJECT_PATH, OrangeIface { tx })
        .await?;
    std::future::pending::<()>().await;
    #[allow(unreachable_code)]
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn metadata_dict_key_types() {
        let meta = MprisMetadata::from_song(
            "So What",
            "Miles Davis",
            "Kind of Blue",
            545_000_000_000,
            3,
            "file:///art.jpg",
        );
        let dict = metadata_dict(&meta);
        // Signature-relevant keys all present.
        for key in [
            "mpris:trackid",
            "mpris:length",
            "xesam:title",
            "xesam:artist",
            "xesam:album",
            "mpris:artUrl",
        ] {
            assert!(dict.contains_key(key), "missing {key}");
        }
        // Track ID keeps object-path type (not a plain string).
        let trackid = dict.get("mpris:trackid").unwrap();
        assert_eq!(trackid.value_signature(), "o");
        assert_eq!(dict.get("xesam:title").unwrap().value_signature(), "s");
        assert_eq!(dict.get("xesam:artist").unwrap().value_signature(), "as");
        assert_eq!(dict.get("mpris:length").unwrap().value_signature(), "x");
    }

    #[test]
    fn invalid_trackid_falls_back() {
        let mut meta = MprisMetadata::from_song("T", "A", "B", 1_000_000, 0, "");
        meta.trackid = "not a path!!".to_string();
        let dict = metadata_dict(&meta);
        assert_eq!(dict.get("mpris:trackid").unwrap().value_signature(), "o");
    }
}
