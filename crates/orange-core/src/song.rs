//! Song record mirroring the 2.1.5 `songs` table columns and defaults.
//! Column list source: `data/schema/schema.sql`.

/// "Unknown" sentinel used by integer columns (`DEFAULT -1` in the schema).
pub const UNKNOWN: i64 = -1;

/// A library song. Only the fields the UI, playlists, and scrobblers need
/// are first-class; the remainder round-trips through [`Song::extra`].
#[derive(Debug, Clone, PartialEq)]
pub struct Song {
    pub title: String,
    pub album: String,
    pub artist: String,
    pub albumartist: String,
    pub track: i64,
    pub disc: i64,
    pub year: i64,
    pub genre: String,
    pub composer: String,
    pub performer: String,
    pub grouping: String,
    pub comment: String,
    pub lyrics: String,
    /// Nanoseconds from start; mirrors `beginning`.
    pub beginning_ns: i64,
    /// Nanoseconds of duration; mirrors `length`.
    pub length_ns: i64,
    pub bitrate: i64,
    pub samplerate: i64,
    pub bitdepth: i64,
    /// File URL; mirrors `url` (NOT NULL in the schema).
    pub url: String,
    pub filesize: i64,
    pub unavailable: bool,
    pub fingerprint: String,
    pub playcount: i64,
    pub skipcount: i64,
    pub lastplayed: i64,
    pub rating: f64,
    pub acoustid_id: String,
    pub musicbrainz_recording_id: String,
    pub ebur128_integrated_loudness_lufs: Option<f64>,
    pub ebur128_loudness_range_lu: Option<f64>,
    pub bpm: Option<f64>,
    pub mood: String,
    pub initial_key: String,
}

impl Default for Song {
    /// Schema defaults: unknown numerics are -1, counts are 0.
    fn default() -> Self {
        Self {
            title: String::new(),
            album: String::new(),
            artist: String::new(),
            albumartist: String::new(),
            track: UNKNOWN,
            disc: UNKNOWN,
            year: UNKNOWN,
            genre: String::new(),
            composer: String::new(),
            performer: String::new(),
            grouping: String::new(),
            comment: String::new(),
            lyrics: String::new(),
            beginning_ns: 0,
            length_ns: 0,
            bitrate: UNKNOWN,
            samplerate: UNKNOWN,
            bitdepth: UNKNOWN,
            url: String::new(),
            filesize: UNKNOWN,
            unavailable: false,
            fingerprint: String::new(),
            playcount: 0,
            skipcount: 0,
            lastplayed: UNKNOWN,
            rating: -1.0,
            acoustid_id: String::new(),
            musicbrainz_recording_id: String::new(),
            ebur128_integrated_loudness_lufs: None,
            ebur128_loudness_range_lu: None,
            bpm: None,
            mood: String::new(),
            initial_key: String::new(),
        }
    }
}

impl Song {
    /// Length in whole seconds, for display and MPRIS (`mpris:length` is µs).
    pub fn length_secs(&self) -> i64 {
        self.length_ns / 1_000_000_000
    }

    /// MPRIS track id suffix: stable per URL, never a local path leak.
    pub fn mpris_trackid(&self, position: usize) -> String {
        format!("/org/mpris/MediaPlayer2/Track/{}", position)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn schema_defaults() {
        let song = Song::default();
        assert_eq!(song.track, -1);
        assert_eq!(song.playcount, 0);
        assert_eq!(song.beginning_ns, 0);
        assert!(song.ebur128_integrated_loudness_lufs.is_none());
    }

    #[test]
    fn length_conversion() {
        let song = Song {
            length_ns: 185_000_000_000,
            ..Song::default()
        };
        assert_eq!(song.length_secs(), 185);
    }
}
