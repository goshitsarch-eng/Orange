//! Tag read/write round-trips (feature `tags`) on a hand-crafted PCM WAV.
//! No fixture audio needed: the WAV container is built byte-exact here, and
//! lofty reads/writes its ID3v2 chunk. Proves real tag editing, not mocks.

#![cfg(feature = "tags")]

use std::path::PathBuf;

use orange_media::tagger::{read_tags, write_tags, FileTags, TagPatch};

/// Minimal valid PCM WAV: 1 s mono 44.1 kHz 16-bit silence.
fn craft_wav_bytes() -> Vec<u8> {
    let sample_rate: u32 = 44_100;
    let samples: u32 = sample_rate;
    let data_len = samples * 2;
    let mut wav = Vec::with_capacity(44 + data_len as usize);
    wav.extend_from_slice(b"RIFF");
    wav.extend_from_slice(&(36 + data_len).to_le_bytes());
    wav.extend_from_slice(b"WAVE");
    wav.extend_from_slice(b"fmt ");
    wav.extend_from_slice(&16u32.to_le_bytes());
    wav.extend_from_slice(&1u16.to_le_bytes()); // PCM
    wav.extend_from_slice(&1u16.to_le_bytes()); // mono
    wav.extend_from_slice(&sample_rate.to_le_bytes());
    wav.extend_from_slice(&(sample_rate * 2).to_le_bytes()); // byte rate
    wav.extend_from_slice(&2u16.to_le_bytes()); // block align
    wav.extend_from_slice(&16u16.to_le_bytes()); // bits
    wav.extend_from_slice(b"data");
    wav.extend_from_slice(&data_len.to_le_bytes());
    wav.extend(std::iter::repeat(0u8).take(data_len as usize));
    wav
}

fn temp_wav(name: &str) -> PathBuf {
    let mut path = std::env::temp_dir();
    path.push(format!("orange-tag-{name}-{}", std::process::id()));
    path.set_extension("wav");
    std::fs::write(&path, craft_wav_bytes()).unwrap();
    path
}

#[test]
fn untagged_wav_reads_empty_with_duration() {
    let path = temp_wav("plain");
    let tags: FileTags = read_tags(&path).unwrap();
    assert!(!tags.has_tag);
    assert_eq!(tags.title, "");
    assert_eq!(tags.duration_secs, 1);
    assert_eq!(tags.format, "Wav");
    std::fs::remove_file(&path).ok();
}

#[test]
fn write_then_read_round_trip() {
    let path = temp_wav("roundtrip");
    write_tags(
        &path,
        &TagPatch {
            title: Some("So What".to_string()),
            artist: Some("Miles Davis".to_string()),
            album: Some("Kind of Blue".to_string()),
            genre: Some("Jazz".to_string()),
            year: Some(1959),
            track: Some(1),
            disc: Some(1),
        },
    )
    .unwrap();
    let tags = read_tags(&path).unwrap();
    assert!(tags.has_tag);
    assert_eq!(tags.title, "So What");
    assert_eq!(tags.artist, "Miles Davis");
    assert_eq!(tags.album, "Kind of Blue");
    assert_eq!(tags.genre, "Jazz");
    assert_eq!(tags.year, Some(1959));
    assert_eq!(tags.track, Some(1));
    assert_eq!(tags.disc, Some(1));
    std::fs::remove_file(&path).ok();
}

#[test]
fn sparse_patch_preserves_other_fields() {
    let path = temp_wav("sparse");
    write_tags(
        &path,
        &TagPatch {
            title: Some("Blue in Green".to_string()),
            artist: Some("Miles Davis".to_string()),
            ..TagPatch::default()
        },
    )
    .unwrap();
    write_tags(
        &path,
        &TagPatch {
            track: Some(3),
            ..TagPatch::default()
        },
    )
    .unwrap();
    let tags = read_tags(&path).unwrap();
    assert_eq!(tags.title, "Blue in Green");
    assert_eq!(tags.artist, "Miles Davis");
    assert_eq!(tags.track, Some(3));
    std::fs::remove_file(&path).ok();
}
