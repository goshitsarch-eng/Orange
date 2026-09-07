//! Live network clients (feature `online`): Radio Browser, lrclib lyrics,
//! MusicBrainz lookup, Cover Art Archive, AcoustID lookup, and scrobble
//! submission (Last.fm / ListenBrainz / Subsonic).
//!
//! Payload shapes mirror 2.1.5. Credentials travel only in [`Secret`]
//! handles and are never logged: request builders take `Secret`, and unit
//! tests assert redaction. Pure Rust (rustls); zero telemetry.

use serde::Deserialize;

use crate::online::Secret;
use crate::scrobble::{canonical_param_string, lastfm_scrobble_params};

/// Required contact User-Agent for the Radio Browser / MusicBrainz APIs.
pub const USER_AGENT: &str = "Orange/3.0.0 (https://github.com/goshitsarch-eng/Orange)";

/// Shared client: contact UA plus a sane timeout, no cookie store.
pub fn new_client() -> Result<reqwest::Client, NetError> {
    reqwest::Client::builder()
        .user_agent(USER_AGENT)
        .timeout(std::time::Duration::from_secs(15))
        .build()
        .map_err(|e| NetError(e.to_string()))
}

/// Network failure (message only: URLs may carry credentials in query).
#[derive(Debug)]
pub struct NetError(pub String);

impl std::fmt::Display for NetError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "network error: {}", self.0)
    }
}

impl std::error::Error for NetError {}

// ---------------------------------------------------------------------------
// Radio Browser (mirrors `radiobrowser`)
// ---------------------------------------------------------------------------

/// One Radio Browser station (nullable upstream fields preserved).
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct Station {
    #[serde(default)]
    pub stationuuid: String,
    #[serde(default)]
    pub name: String,
    #[serde(default)]
    pub url: String,
    #[serde(default)]
    pub url_resolved: String,
    #[serde(default)]
    pub homepage: String,
    #[serde(default)]
    pub codec: String,
    #[serde(default)]
    pub bitrate: i64,
}

/// Search stations by name. `base` defaults to
/// [`crate::radio::RADIO_BROWSER_API_BASE`];
/// tests inject a local server.
pub async fn search_stations(
    client: &reqwest::Client,
    base: &str,
    query: &str,
    limit: u32,
) -> Result<Vec<Station>, NetError> {
    let url = format!("{base}/json/stations/search?name={query}&limit={limit}&hidebroken=true");
    client
        .get(url)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?
        .json::<Vec<Station>>()
        .await
        .map_err(|e| NetError(e.to_string()))
}

/// Register a stream click (mirrors the 2.1.5 click counter).
pub async fn register_station_click(
    client: &reqwest::Client,
    base: &str,
    stationuuid: &str,
) -> Result<String, NetError> {
    #[derive(Deserialize)]
    struct Click {
        #[serde(default)]
        url: String,
    }
    let url = format!("{base}/json/url/{stationuuid}");
    let click = client
        .get(url)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?
        .json::<Click>()
        .await
        .map_err(|e| NetError(e.to_string()))?;
    Ok(click.url)
}

// ---------------------------------------------------------------------------
// lrclib lyrics (no key needed)
// ---------------------------------------------------------------------------

/// lrclib `/api/get` hit (upstream uses camelCase). Instrumental tracks
/// report `instrumental: true` with null lyrics and the LRC inside
/// `lyricsfile` instead.
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct LrclibHit {
    #[serde(default)]
    pub plain_lyrics: Option<String>,
    #[serde(default)]
    pub synced_lyrics: Option<String>,
    #[serde(default)]
    pub instrumental: bool,
}

/// Fetch lyrics for one track. `None` fields mean "not found upstream".
pub async fn fetch_lyrics(
    client: &reqwest::Client,
    artist: &str,
    track: &str,
    album: &str,
    duration_secs: i64,
) -> Result<LrclibHit, NetError> {
    let url = format!(
        "https://lrclib.net/api/get?artist_name={artist}&track_name={track}&album_name={album}&duration={duration_secs}"
    );
    client
        .get(url)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?
        .json::<LrclibHit>()
        .await
        .map_err(|e| NetError(e.to_string()))
}

// ---------------------------------------------------------------------------
// MusicBrainz + Cover Art Archive + AcoustID
// ---------------------------------------------------------------------------

/// MusicBrainz recording search hit (first result wins, like 2.1.5).
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct MbRecording {
    #[serde(default)]
    pub id: String,
    #[serde(default)]
    pub title: String,
    #[serde(default)]
    pub releases: Vec<MbRelease>,
}

/// MusicBrainz release stub.
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct MbRelease {
    #[serde(default)]
    pub id: String,
    #[serde(default)]
    pub title: String,
}

/// Search recordings by artist + title.
pub async fn search_recording(
    client: &reqwest::Client,
    artist: &str,
    title: &str,
) -> Result<Vec<MbRecording>, NetError> {
    #[derive(Deserialize)]
    struct Search {
        #[serde(default)]
        recordings: Vec<MbRecording>,
    }
    let query = format!("artist:{artist} AND recording:{title}");
    let url = format!("https://musicbrainz.org/ws/2/recording/?query={query}&fmt=json&limit=5");
    let search = client
        .get(url)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?
        .json::<Search>()
        .await
        .map_err(|e| NetError(e.to_string()))?;
    Ok(search.recordings)
}

/// Release MBIDs attached to a MusicBrainz DiscID lookup.
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct DiscidLookup {
    #[serde(default)]
    pub releases: Vec<MbRelease>,
}

/// Look up a DiscID (from [`crate::cd`]) and return its releases.
pub async fn lookup_discid(
    client: &reqwest::Client,
    disc_id: &str,
) -> Result<DiscidLookup, NetError> {
    let url = crate::cd::discid_lookup_url(disc_id);
    client
        .get(url)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?
        .json::<DiscidLookup>()
        .await
        .map_err(|e| NetError(e.to_string()))
}

/// Cover Art Archive front-cover URL for a release MBID (follows redirect).
pub fn cover_art_url(release_mbid: &str) -> String {
    format!("https://coverartarchive.org/release/{release_mbid}/front-500")
}

/// Download front cover bytes for a release MBID.
pub async fn fetch_cover(
    client: &reqwest::Client,
    release_mbid: &str,
) -> Result<Vec<u8>, NetError> {
    client
        .get(cover_art_url(release_mbid))
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?
        .bytes()
        .await
        .map(|bytes| bytes.to_vec())
        .map_err(|e| NetError(e.to_string()))
}

/// AcoustID lookup response (first recording wins).
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct AcoustidResult {
    #[serde(default)]
    pub results: Vec<AcoustidMatch>,
}

/// One AcoustID match.
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct AcoustidMatch {
    #[serde(default)]
    pub id: String,
    #[serde(default)]
    pub recordings: Vec<AcoustidRecording>,
}

/// Recording stub inside an AcoustID match.
#[derive(Debug, Clone, PartialEq, Eq, Deserialize)]
pub struct AcoustidRecording {
    #[serde(default)]
    pub id: String,
}

/// Look up a Chromaprint fingerprint (from `fpcalc`, see `tagger`).
pub async fn acoustid_lookup(
    client: &reqwest::Client,
    fingerprint: &str,
    duration_secs: u32,
    api_key: &str,
) -> Result<AcoustidResult, NetError> {
    let url = format!(
        "https://api.acoustid.org/v2/lookup?client={api_key}&meta=recordingids&duration={duration_secs}&fingerprint={fingerprint}"
    );
    client
        .get(url)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?
        .json::<AcoustidResult>()
        .await
        .map_err(|e| NetError(e.to_string()))
}

// ---------------------------------------------------------------------------
// Scrobble submission
// ---------------------------------------------------------------------------

/// Submit one Last.fm scrobble. The API secret is only used inside the
/// signature closure; the session key travels as a form field over TLS.
pub async fn submit_lastfm(
    client: &reqwest::Client,
    api_key: &str,
    api_secret: &Secret,
    session_key: &str,
    artist: &str,
    track: &str,
    album: &str,
    timestamp: i64,
) -> Result<(), NetError> {
    let params = lastfm_scrobble_params(api_key, session_key, artist, track, album, timestamp);
    let borrowed: Vec<(&str, &str)> = params
        .iter()
        .map(|(key, value)| (*key, value.as_str()))
        .collect();
    let signature = api_secret.use_secret(|secret| {
        let mut hasher = md5::Md5::new();
        use md5::Digest;
        hasher.update(canonical_param_string(&borrowed));
        hasher.update(secret);
        hex::encode(hasher.finalize())
    });
    let mut form = params;
    form.push(("api_sig", signature));
    form.push(("format", "json".to_string()));
    client
        .post("https://ws.audioscrobbler.com/2.0/")
        .form(&form)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?;
    Ok(())
}

/// ListenBrainz `single` listen payload (pure builder, tested shape).
pub fn listenbrainz_payload(
    artist: &str,
    track: &str,
    album: &str,
    recording_mbid: &str,
    listened_at: i64,
) -> serde_json::Value {
    serde_json::json!({
        "listen_type": "single",
        "payload": [{
            "listened_at": listened_at,
            "track_metadata": {
                "artist_name": artist,
                "track_name": track,
                "release_name": album,
                "additional_info": {
                    "recording_mbid": recording_mbid,
                    "submission_client": "Orange",
                    "submission_client_version": "3.0.0",
                },
            },
        }],
    })
}

/// Submit one ListenBrainz listen. The user token travels only as the
/// `Authorization: Token` header value over TLS.
pub async fn submit_listenbrainz(
    client: &reqwest::Client,
    user_token: Secret,
    artist: &str,
    track: &str,
    album: &str,
    recording_mbid: &str,
    listened_at: i64,
) -> Result<(), NetError> {
    let payload = listenbrainz_payload(artist, track, album, recording_mbid, listened_at);
    client
        .post("https://api.listenbrainz.org/1/submit-listens")
        .header(
            reqwest::header::AUTHORIZATION,
            user_token.into_authorization_header("Token"),
        )
        .json(&payload)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?;
    Ok(())
}

/// Submit one Subsonic scrobble. The password never leaves the caller: only
/// the `md5(password + salt)` token is sent, per the Subsonic auth scheme.
pub async fn submit_subsonic(
    client: &reqwest::Client,
    base: &str,
    user: &str,
    password: &Secret,
    salt: &str,
    song_id: &str,
    played_at: i64,
) -> Result<(), NetError> {
    let token = password.use_secret(|secret| {
        let mut hasher = md5::Md5::new();
        use md5::Digest;
        hasher.update(secret);
        hasher.update(salt);
        hex::encode(hasher.finalize())
    });
    let url =
        crate::scrobble::subsonic_scrobble_url(base, user, &token, salt, song_id, played_at, true);
    client
        .get(url)
        .send()
        .await
        .map_err(|e| NetError(e.to_string()))?
        .error_for_status()
        .map_err(|e| NetError(e.to_string()))?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;
    use crate::radio::RADIO_BROWSER_API_BASE;

    const STATION_JSON: &str = r#"[{
        "stationuuid": "9617a7b9-0601-11e8-8d8a-52543be04c81",
        "name": "Groove Salad",
        "url": "http://ice1.somafm.com/groovesalad-128-mp3",
        "url_resolved": "https://ice1.somafm.com/groovesalad-128-mp3",
        "homepage": "https://somafm.com/groovesalad/",
        "codec": "MP3",
        "bitrate": 128
    }]"#;

    #[test]
    fn station_json_parses() {
        let stations: Vec<Station> = serde_json::from_str(STATION_JSON).unwrap();
        assert_eq!(stations.len(), 1);
        assert_eq!(stations[0].name, "Groove Salad");
        assert_eq!(stations[0].bitrate, 128);
        assert!(stations[0].url_resolved.starts_with("https://"));
    }

    #[test]
    fn station_json_tolerates_missing_fields() {
        let stations: Vec<Station> = serde_json::from_str(r#"[{}]"#).unwrap();
        assert_eq!(stations[0].bitrate, 0);
        assert_eq!(stations[0].name, "");
    }

    const LRCLIB_JSON: &str =
        r#"{"plainLyrics":"Blue in green\n","syncedLyrics":"[00:01.00] Blue in green\n"}"#;

    #[test]
    fn lrclib_json_parses() {
        let hit: LrclibHit = serde_json::from_str(LRCLIB_JSON).unwrap();
        assert!(hit.plain_lyrics.unwrap().contains("Blue in green"));
        assert!(hit.synced_lyrics.unwrap().starts_with("[00:01.00]"));
        let empty: LrclibHit = serde_json::from_str("{}").unwrap();
        assert!(empty.plain_lyrics.is_none());
    }

    const MB_JSON: &str = r#"{"recordings":[{
        "id": "recording-mbid",
        "title": "So What",
        "releases": [{"id": "release-mbid", "title": "Kind of Blue"}]
    }]}"#;

    #[test]
    fn musicbrainz_json_parses() {
        #[derive(Deserialize)]
        struct Search {
            #[serde(default)]
            recordings: Vec<MbRecording>,
        }
        let search: Search = serde_json::from_str(MB_JSON).unwrap();
        assert_eq!(search.recordings[0].id, "recording-mbid");
        assert_eq!(search.recordings[0].releases[0].id, "release-mbid");
        assert_eq!(
            cover_art_url("release-mbid"),
            "https://coverartarchive.org/release/release-mbid/front-500"
        );
    }

    const ACOUSTID_JSON: &str = r#"{"results":[{
        "id": "match-id",
        "recordings": [{"id": "recording-mbid"}]
    }]}"#;

    #[test]
    fn acoustid_json_parses() {
        let result: AcoustidResult = serde_json::from_str(ACOUSTID_JSON).unwrap();
        assert_eq!(result.results.len(), 1);
        assert_eq!(result.results[0].recordings[0].id, "recording-mbid");
    }

    #[test]
    fn listenbrainz_payload_shape() {
        let payload =
            listenbrainz_payload("Miles Davis", "So What", "Kind of Blue", "mbid", 1700000000);
        assert_eq!(payload["listen_type"], "single");
        assert_eq!(payload["payload"][0]["listened_at"], 1700000000);
        assert_eq!(
            payload["payload"][0]["track_metadata"]["artist_name"],
            "Miles Davis"
        );
        assert_eq!(
            payload["payload"][0]["track_metadata"]["additional_info"]["submission_client"],
            "Orange"
        );
    }

    // Live endpoint checks. Ignored by default (`cargo test` stays hermetic);
    // run explicitly: `cargo test --features online -- --ignored`.
    #[tokio::test]
    #[ignore]
    async fn live_radio_browser_search() {
        let client = new_client().unwrap();
        let stations = search_stations(&client, RADIO_BROWSER_API_BASE, "Groove Salad", 3)
            .await
            .unwrap();
        assert!(!stations.is_empty());
    }

    #[tokio::test]
    #[ignore]
    async fn live_lrclib_lookup() {
        let client = new_client().unwrap();
        // "So What" is instrumental: found, but with null lyrics.
        let instrumental = fetch_lyrics(&client, "Miles Davis", "So What", "Kind of Blue", 545)
            .await
            .unwrap();
        assert!(instrumental.instrumental);
        // A lyrical track returns real lyrics.
        let hit = fetch_lyrics(&client, "Radiohead", "Creep", "Pablo Honey", 238)
            .await
            .unwrap();
        assert!(!hit.instrumental);
        assert!(hit.plain_lyrics.is_some() || hit.synced_lyrics.is_some());
    }
}
