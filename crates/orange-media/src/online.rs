//! Online services: scrobbling, cover/lyrics providers, streaming.
//! Mirrors `scrobbler`, `covermanager`, `lyrics`, `streaming`, `subsonic`,
//! `tidal`, `qobuz`, `spotify`, and `tagfetcher` (AcoustID + MusicBrainz).
//!
//! Privacy rules, enforced by construction:
//! - Credentials live in OS-keyring-shaped opaque [`Secret`] handles and are
//!   never `Display`/`Debug`-printed, never logged, never serialized.
//! - Unofficial Tidal/Spotify/Qobuz integrations behave as in 2.1.5.
//! - Zero telemetry: no usage, crash, or analytics reporting of any kind.

use std::fmt;

/// An opaque credential handle. Deliberately has no content accessors on the
/// hot path: only the authenticated request builder consumes it.
#[derive(Clone)]
pub struct Secret {
    inner: String,
}

impl Secret {
    pub fn new(secret: impl Into<String>) -> Self {
        Self {
            inner: secret.into(),
        }
    }

    /// Consume the secret into an `Authorization` header value. The only
    /// sanctioned way the bytes leave the handle.
    pub fn into_authorization_header(self, scheme: &str) -> String {
        format!("{scheme} {}", self.inner)
    }

    /// Run `f` with the raw secret (for HMAC/MD5 signing flows such as
    /// Last.fm `api_sig` and Subsonic tokens). The bytes never escape except
    /// through `f`'s return value, which must be a signature, never the key.
    pub fn use_secret<R>(&self, f: impl FnOnce(&str) -> R) -> R {
        f(&self.inner)
    }
}

impl fmt::Debug for Secret {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        f.write_str("Secret([redacted])")
    }
}

/// Scrobble services, mirroring the 2.1.5 scrobbler settings.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum ScrobbleService {
    LastFm,
    ListenBrainz,
    Subsonic,
}

impl ScrobbleService {
    pub fn name(self) -> &'static str {
        match self {
            Self::LastFm => "Last.fm",
            Self::ListenBrainz => "ListenBrainz",
            Self::Subsonic => "Subsonic",
        }
    }
}

/// One scrobble event. Contains metadata only, never credentials.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Scrobble {
    pub service: ScrobbleService,
    pub artist: String,
    pub title: String,
    pub album: String,
    pub track_mbid: String,
    /// Unix timestamp when playback started.
    pub started_at: i64,
}

impl Scrobble {
    /// 2.1.5 rule: scrobble when played past half the track or 4 minutes.
    pub fn should_scrobble(played_secs: i64, length_secs: i64) -> bool {
        if length_secs <= 30 {
            return false;
        }
        played_secs >= (length_secs / 2).min(4 * 60)
    }
}

/// Cover providers, mirroring `covermanager` (2.1.5 order).
pub const COVER_PROVIDERS: &[&str] = &[
    "Last.fm",
    "MusicBrainz",
    "Discogs",
    "Musixmatch",
    "Deezer",
    "Tidal",
    "Qobuz",
    "Spotify",
];

/// Lyrics providers, mirroring `lyrics` (2.1.5 set).
pub const LYRICS_PROVIDERS: &[&str] = &[
    "Genius",
    "Musixmatch",
    "lyrics.ovh",
    "songlyrics.com",
    "azlyrics.com",
    "elyrics.net",
    "letras.com",
    "lrclib.net",
];

/// Streaming integrations, mirroring `streaming` + unofficial clients.
/// Kept as-is from 2.1.5; credentials use [`Secret`].
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum StreamingService {
    Subsonic,
    Tidal,
    Qobuz,
    Spotify,
}

impl StreamingService {
    pub fn name(self) -> &'static str {
        match self {
            Self::Subsonic => "Subsonic",
            Self::Tidal => "Tidal",
            Self::Qobuz => "Qobuz",
            Self::Spotify => "Spotify",
        }
    }

    pub fn is_unofficial(self) -> bool {
        !matches!(self, Self::Subsonic)
    }
}

/// Tag fetch request: AcoustID fingerprint resolved via MusicBrainz.
/// Mirrors `tagfetcher`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TagFetchRequest {
    pub acoustid_fingerprint: String,
    pub duration_secs: i64,
}

impl TagFetchRequest {
    pub fn is_valid(&self) -> bool {
        !self.acoustid_fingerprint.is_empty() && self.duration_secs > 0
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn scoped_reveal_keeps_debug_redacted() {
        let secret = Secret::new("s3cr3t");
        let len = secret.use_secret(str::len);
        assert_eq!(len, 6);
        assert_eq!(format!("{secret:?}"), "Secret([redacted])");
    }

    #[test]
    fn secret_never_prints() {
        let secret = Secret::new("hunter2");
        assert_eq!(format!("{secret:?}"), "Secret([redacted])");
        assert!(!format!("{secret:?}").contains("hunter2"));
        let header = secret.into_authorization_header("Bearer");
        assert_eq!(header, "Bearer hunter2");
    }

    #[test]
    fn scrobble_threshold() {
        assert!(Scrobble::should_scrobble(120, 200));
        assert!(!Scrobble::should_scrobble(30, 200));
        assert!(!Scrobble::should_scrobble(200, 20));
        // Long podcast: 4 minutes suffices.
        assert!(Scrobble::should_scrobble(240, 3600));
        assert!(!Scrobble::should_scrobble(239, 3600));
    }

    #[test]
    fn provider_lists_match_2_1_5() {
        assert_eq!(COVER_PROVIDERS.len(), 8);
        assert!(COVER_PROVIDERS.contains(&"Spotify"));
        assert_eq!(LYRICS_PROVIDERS.len(), 8);
        assert!(LYRICS_PROVIDERS.contains(&"lrclib.net"));
        assert!(StreamingService::Tidal.is_unofficial());
        assert!(!StreamingService::Subsonic.is_unofficial());
    }

    #[test]
    fn tag_fetch_validation() {
        assert!(TagFetchRequest {
            acoustid_fingerprint: "abc".into(),
            duration_secs: 185
        }
        .is_valid());
        assert!(!TagFetchRequest {
            acoustid_fingerprint: String::new(),
            duration_secs: 185
        }
        .is_valid());
    }
}
