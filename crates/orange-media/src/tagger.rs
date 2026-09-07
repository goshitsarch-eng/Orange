//! Tag reading/writing (feature `tags`) via lofty: pure Rust, no TagLib.
//! Mirrors `tagreader` plus the tag editor save path.
//!
//! Fingerprinting shells out to `fpcalc` (Chromaprint) when installed and
//! parses its JSON without extra dependencies; MusicBrainz/AcoustID lookup
//! on top of the fingerprint lives in `net` (feature `online`).

use std::path::Path;

use lofty::config::WriteOptions;
use lofty::file::AudioFile;
use lofty::prelude::{Accessor, TaggedFileExt};
use lofty::probe::Probe;
use lofty::tag::Tag;

/// Tag backend failure.
#[derive(Debug)]
pub struct TagError(pub String);

impl std::fmt::Display for TagError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "tag error: {}", self.0)
    }
}

impl std::error::Error for TagError {}

impl From<lofty::error::LoftyError> for TagError {
    fn from(e: lofty::error::LoftyError) -> Self {
        Self(e.to_string())
    }
}

impl From<std::io::Error> for TagError {
    fn from(e: std::io::Error) -> Self {
        Self(e.to_string())
    }
}

/// Tags read from one audio file. String fields are empty when absent.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct FileTags {
    pub title: String,
    pub artist: String,
    pub album: String,
    pub genre: String,
    pub year: Option<u32>,
    pub track: Option<u32>,
    pub disc: Option<u32>,
    pub duration_secs: u64,
    /// lofty format name (e.g. `"Wav"`, `"Mpeg"`, `"Flac"`).
    pub format: String,
    /// False when the file carries no tag at all.
    pub has_tag: bool,
}

/// Sparse tag edit: only `Some` fields are written, the rest are untouched.
#[derive(Debug, Clone, Default, PartialEq, Eq)]
pub struct TagPatch {
    pub title: Option<String>,
    pub artist: Option<String>,
    pub album: Option<String>,
    pub genre: Option<String>,
    pub year: Option<u32>,
    pub track: Option<u32>,
    pub disc: Option<u32>,
}

/// Read tags + properties from `path`.
pub fn read_tags(path: &Path) -> Result<FileTags, TagError> {
    let tagged = Probe::open(path)?.read()?;
    let duration_secs = tagged.properties().duration().as_secs();
    let format = format!("{:?}", tagged.file_type());
    let Some(tag) = tagged.primary_tag() else {
        return Ok(FileTags {
            duration_secs,
            format,
            has_tag: false,
            ..FileTags::default()
        });
    };
    Ok(FileTags {
        title: tag.title().map(into_owned).unwrap_or_default(),
        artist: tag.artist().map(into_owned).unwrap_or_default(),
        album: tag.album().map(into_owned).unwrap_or_default(),
        genre: tag.genre().map(into_owned).unwrap_or_default(),
        year: tag.year(),
        track: tag.track(),
        disc: tag.disk(),
        duration_secs,
        format,
        has_tag: true,
    })
}

fn into_owned(value: std::borrow::Cow<'_, str>) -> String {
    value.into_owned()
}

/// Write `patch` into `path`, creating a primary tag when missing.
/// Only `Some` fields change; everything else is preserved byte-wise by
/// lofty's in-place update.
pub fn write_tags(path: &Path, patch: &TagPatch) -> Result<(), TagError> {
    let mut tagged = Probe::open(path)?.read()?;
    if tagged.primary_tag().is_none() {
        let tag_type = tagged.file_type().primary_tag_type();
        tagged.insert_tag(Tag::new(tag_type));
    }
    let tag = tagged
        .primary_tag_mut()
        .ok_or_else(|| TagError("no primary tag after insert".to_string()))?;
    if let Some(title) = &patch.title {
        tag.set_title(title.clone());
    }
    if let Some(artist) = &patch.artist {
        tag.set_artist(artist.clone());
    }
    if let Some(album) = &patch.album {
        tag.set_album(album.clone());
    }
    if let Some(genre) = &patch.genre {
        tag.set_genre(genre.clone());
    }
    if let Some(year) = patch.year {
        tag.set_year(year);
    }
    if let Some(track) = patch.track {
        tag.set_track(track);
    }
    if let Some(disc) = patch.disc {
        tag.set_disk(disc);
    }
    let mut file = std::fs::OpenOptions::new()
        .read(true)
        .write(true)
        .open(path)?;
    tagged.save_to(&mut file, WriteOptions::default())?;
    Ok(())
}

/// True when lofty can tag files with this extension (MP3, FLAC, ...).
pub fn can_tag_extension(ext: &str) -> bool {
    lofty::file::FileType::from_ext(ext.trim_start_matches('.')).is_some()
}

/// Chromaprint fingerprint from `fpcalc -json`, when installed.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Fingerprint {
    pub fingerprint: String,
    pub duration_secs: u32,
}

/// Parse `fpcalc -json` output (`{"duration":N,"fingerprint":"..."}`).
/// Hand-rolled: the shape is fixed and tiny.
pub fn parse_fpcalc_json(json: &str) -> Option<Fingerprint> {
    let duration = json_field_u32(json, "\"duration\"")?;
    let fingerprint = json_field_string(json, "\"fingerprint\"")?;
    if fingerprint.is_empty() {
        return None;
    }
    Some(Fingerprint {
        fingerprint,
        duration_secs: duration,
    })
}

fn json_field_u32(json: &str, key: &str) -> Option<u32> {
    let start = json.find(key)? + key.len();
    let rest = json[start..].trim_start_matches([' ', ':', '\t']);
    rest.split(|c: char| !c.is_ascii_digit())
        .next()?
        .parse()
        .ok()
}

/// Run `fpcalc -json <path>` (Chromaprint tools). Err when missing/failing.
pub fn fingerprint_with_fpcalc(path: &Path) -> Result<Fingerprint, TagError> {
    let output = std::process::Command::new("fpcalc")
        .arg("-json")
        .arg(path)
        .output()
        .map_err(|e| TagError(format!("fpcalc not available: {e}")))?;
    if !output.status.success() {
        return Err(TagError(format!(
            "fpcalc failed: {}",
            String::from_utf8_lossy(&output.stderr).trim()
        )));
    }
    let stdout = String::from_utf8_lossy(&output.stdout);
    parse_fpcalc_json(&stdout).ok_or_else(|| TagError("fpcalc JSON unparsable".to_string()))
}

fn json_field_string(json: &str, key: &str) -> Option<String> {
    let start = json.find(key)? + key.len();
    let rest = json[start..].trim_start_matches([' ', ':', '\t', '\n', '\r']);
    let rest = rest.strip_prefix('"')?;
    let mut out = String::new();
    let mut chars = rest.chars();
    while let Some(c) = chars.next() {
        match c {
            '\\' => out.push(chars.next()?),
            '"' => return Some(out),
            _ => out.push(c),
        }
    }
    None
}

/// AcoustID lookup URL for a fingerprint (key kept by the caller).
/// Mirrors the 2.1.5 tagfetcher request.
pub fn acoustid_lookup_url(fingerprint: &Fingerprint, api_key: &str) -> String {
    format!(
        "https://api.acoustid.org/v2/lookup?client={api_key}&meta=recordingids+releaseids&duration={}&fingerprint={}",
        fingerprint.duration_secs, fingerprint.fingerprint
    )
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn fpcalc_json_parses() {
        let parsed = parse_fpcalc_json("{\"duration\":185,\"fingerprint\":\"AQAAAA\"}").unwrap();
        assert_eq!(
            parsed,
            Fingerprint {
                fingerprint: "AQAAAA".to_string(),
                duration_secs: 185
            }
        );
        assert!(parse_fpcalc_json("{\"duration\":185}").is_none());
        assert!(parse_fpcalc_json("not json").is_none());
        assert!(parse_fpcalc_json("{\"duration\":0,\"fingerprint\":\"\"}").is_none());
    }

    #[test]
    fn acoustid_url_shape() {
        let fingerprint = Fingerprint {
            fingerprint: "ABC".to_string(),
            duration_secs: 200,
        };
        let url = acoustid_lookup_url(&fingerprint, "KEY");
        assert!(url.starts_with("https://api.acoustid.org/v2/lookup?"));
        assert!(url.contains("duration=200"));
        assert!(!url.contains("hunter2"));
    }

    #[test]
    fn taggable_extensions() {
        assert!(can_tag_extension("mp3"));
        assert!(can_tag_extension(".flac"));
        assert!(can_tag_extension("ogg"));
        assert!(!can_tag_extension("exe"));
        assert!(!can_tag_extension(""));
    }
}
