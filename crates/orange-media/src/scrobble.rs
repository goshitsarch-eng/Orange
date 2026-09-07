//! Scrobble request builders (no network, no credentials in output).
//!
//! Pure constructors for Last.fm, ListenBrainz, and Subsonic scrobbles.
//! Live submission lives in `net` (feature `online`); the MD5 signature step
//! needs feature `crypto`. Mirrors the 2.1.5 scrobbler payload shapes.

/// Last.fm `api_sig` input: parameters sorted by key, concatenated as
/// `key+value` with no separator. Returns the exact string that gets hashed.
pub fn canonical_param_string(params: &[(&str, &str)]) -> String {
    let mut sorted: Vec<(&str, &str)> = params.to_vec();
    sorted.sort_by(|a, b| a.0.cmp(b.0));
    let mut out = String::new();
    for (key, value) in sorted {
        out.push_str(key);
        out.push_str(value);
    }
    out
}

/// Last.fm `api_sig`: MD5 of `canonical_param_string(params) + api_secret`.
#[cfg(feature = "crypto")]
pub fn lastfm_signature(params: &[(&str, &str)], api_secret: &str) -> String {
    use md5::{Digest, Md5};
    let mut hasher = Md5::new();
    hasher.update(canonical_param_string(params));
    hasher.update(api_secret);
    hex::encode(hasher.finalize())
}

/// Last.fm `track.scrobble` parameters (before signing). Timestamps are Unix
/// seconds, matching the 2.1.5 payload.
pub fn lastfm_scrobble_params(
    api_key: &str,
    session_key: &str,
    artist: &str,
    track: &str,
    album: &str,
    timestamp: i64,
) -> Vec<(&'static str, String)> {
    vec![
        ("method", "track.scrobble".to_string()),
        ("api_key", api_key.to_string()),
        ("sk", session_key.to_string()),
        ("artist", artist.to_string()),
        ("track", track.to_string()),
        ("album", album.to_string()),
        ("timestamp", timestamp.to_string()),
    ]
}

/// Subsonic `scrobble` endpoint URL. `token` is the precomputed
/// `md5(password + salt)`; this builder never sees the password itself.
pub fn subsonic_scrobble_url(
    base: &str,
    user: &str,
    token: &str,
    salt: &str,
    song_id: &str,
    played_at: i64,
    submission: bool,
) -> String {
    let base = base.trim_end_matches('/');
    format!(
        "{base}/rest/scrobble?id={song_id}&time={played_at}&submission={}&u={user}&t={token}&s={salt}&v=1.16.1&c=Orange&f=json",
        if submission { "true" } else { "false" }
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn canonical_string_matches_oracle() {
        // Independent oracle: same concatenation computed with CPython.
        let params = [
            ("api_key", "APIKEY"),
            ("artist", "Miles Davis"),
            ("method", "track.scrobble"),
            ("sk", "SESSION"),
            ("timestamp", "1700000000"),
            ("track", "So What"),
        ];
        assert_eq!(
            canonical_param_string(&params),
            "api_keyAPIKEYartistMiles Davismethodtrack.scrobbleskSESSIONtimestamp1700000000trackSo What"
        );
    }

    #[cfg(feature = "crypto")]
    #[test]
    fn lastfm_signature_matches_oracle() {
        // Oracle: CPython hashlib.md5 of canonical + "SECRET".
        let params = [
            ("api_key", "APIKEY"),
            ("artist", "Miles Davis"),
            ("method", "track.scrobble"),
            ("sk", "SESSION"),
            ("timestamp", "1700000000"),
            ("track", "So What"),
        ];
        assert_eq!(
            lastfm_signature(&params, "SECRET"),
            "55bb6d7d95bc11ac989a5dd3cdabbb3a"
        );
    }

    #[test]
    fn subsonic_url_shape() {
        let url = subsonic_scrobble_url(
            "https://music.example.com/",
            "gosh",
            "tok",
            "sal",
            "42",
            1700000000,
            true,
        );
        assert_eq!(
            url,
            "https://music.example.com/rest/scrobble?id=42&time=1700000000&submission=true&u=gosh&t=tok&s=sal&v=1.16.1&c=Orange&f=json"
        );
    }

    #[test]
    fn scrobble_params_carry_metadata() {
        let params = lastfm_scrobble_params("K", "S", "Miles Davis", "So What", "Kind of Blue", 7);
        let get = |key: &str| {
            params
                .iter()
                .find(|(k, _)| *k == key)
                .map(|(_, v)| v.clone())
                .unwrap()
        };
        assert_eq!(get("method"), "track.scrobble");
        assert_eq!(get("timestamp"), "7");
        assert_eq!(get("album"), "Kind of Blue");
    }
}
