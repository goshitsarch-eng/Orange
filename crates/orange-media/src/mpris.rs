//! MPRIS2 remote interface over D-Bus.
//! Mirrors `mpris2`: bus name, object path, and the metadata map the player
//! publishes. The live `zbus` connection lives in the shell; this module is
//! the pure mapping both sides share.

/// Well-known bus name prefix; the full name is `org.mpris.MediaPlayer2.orange`.
pub const BUS_NAME: &str = "org.mpris.MediaPlayer2.orange";
/// Object path all MPRIS objects live under.
pub const OBJECT_PATH: &str = "/org/mpris/MediaPlayer2";
/// Desktop entry hint for clients.
pub const DESKTOP_ENTRY: &str = "com.goshapps.Orange";
/// Our identity string on the bus.
pub const IDENTITY: &str = "Orange Music Player";

/// Playback status as MPRIS clients see it.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub enum PlaybackStatus {
    Playing,
    Paused,
    #[default]
    Stopped,
}

impl PlaybackStatus {
    pub fn as_str(self) -> &'static str {
        match self {
            Self::Playing => "Playing",
            Self::Paused => "Paused",
            Self::Stopped => "Stopped",
        }
    }
}

/// Track metadata in MPRIS shape. `trackid` is `/org/mpris/...` scoped;
/// `length_microseconds` follows the spec (`mpris:length` in µs).
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct MprisMetadata {
    pub trackid: String,
    pub title: String,
    pub artist: Vec<String>,
    pub album: String,
    pub length_microseconds: i64,
    pub art_url: String,
}

impl MprisMetadata {
    pub fn from_song(
        title: &str,
        artist: &str,
        album: &str,
        length_ns: i64,
        position: usize,
        art_url: &str,
    ) -> Self {
        Self {
            trackid: format!("/org/mpris/MediaPlayer2/Track/{position}"),
            title: title.to_string(),
            artist: if artist.is_empty() {
                Vec::new()
            } else {
                vec![artist.to_string()]
            },
            album: album.to_string(),
            length_microseconds: length_ns / 1_000,
            art_url: art_url.to_string(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn bus_identity() {
        assert_eq!(BUS_NAME, "org.mpris.MediaPlayer2.orange");
        assert_eq!(OBJECT_PATH, "/org/mpris/MediaPlayer2");
        assert_eq!(DESKTOP_ENTRY, "com.goshapps.Orange");
        assert_eq!(PlaybackStatus::Playing.as_str(), "Playing");
    }

    #[test]
    fn metadata_units() {
        let meta = MprisMetadata::from_song(
            "So What",
            "Miles Davis",
            "Kind of Blue",
            545_000_000_000,
            3,
            "",
        );
        assert_eq!(meta.length_microseconds, 545_000_000);
        assert_eq!(meta.trackid, "/org/mpris/MediaPlayer2/Track/3");
        assert_eq!(meta.artist, vec!["Miles Davis".to_string()]);
    }
}
