//! Codec and transcode tables, mirroring the 2.1.5 feature set.
//!
//! Playback: WAV, FLAC, Ogg FLAC, WavPack, Ogg Vorbis, Opus, Ogg Speex, MPC,
//! TrueAudio, AIFF, MP4/AAC, ALAC, MP3, ASF, Monkey's Audio, DSD (DSF/DSDIFF).
//! Transcode targets: MP3, AAC, FLAC, Ogg Vorbis, Opus, Speex, WavPack, ASF.

/// A decodable input format.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct Codec {
    /// Display name, e.g. "Ogg Vorbis".
    pub name: &'static str,
    /// Typical file extensions (lowercase, no dot).
    pub extensions: &'static [&'static str],
}

/// Every input codec Orange 3 plays, in 2.1.5 documentation order.
pub const SUPPORTED_CODECS: &[Codec] = &[
    Codec {
        name: "WAV",
        extensions: &["wav"],
    },
    Codec {
        name: "FLAC",
        extensions: &["flac"],
    },
    Codec {
        name: "Ogg FLAC",
        extensions: &["oga", "ogg"],
    },
    Codec {
        name: "WavPack",
        extensions: &["wv"],
    },
    Codec {
        name: "Ogg Vorbis",
        extensions: &["ogg", "oga"],
    },
    Codec {
        name: "Opus",
        extensions: &["opus"],
    },
    Codec {
        name: "Ogg Speex",
        extensions: &["spx", "ogg"],
    },
    Codec {
        name: "MPC",
        extensions: &["mpc", "mp+", "mpp"],
    },
    Codec {
        name: "TrueAudio",
        extensions: &["tta"],
    },
    Codec {
        name: "AIFF",
        extensions: &["aiff", "aif", "afc"],
    },
    Codec {
        name: "MP4/AAC",
        extensions: &["m4a", "m4b", "m4p", "aac"],
    },
    Codec {
        name: "ALAC",
        extensions: &["m4a"],
    },
    Codec {
        name: "MP3",
        extensions: &["mp3"],
    },
    Codec {
        name: "ASF",
        extensions: &["wma", "asf"],
    },
    Codec {
        name: "Monkey's Audio",
        extensions: &["ape", "mac"],
    },
    Codec {
        name: "DSD (DSF)",
        extensions: &["dsf"],
    },
    Codec {
        name: "DSD (DSDIFF)",
        extensions: &["dff"],
    },
];

/// A transcode output target.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct TranscodeTarget {
    pub name: &'static str,
    pub extension: &'static str,
}

/// Every transcode target Orange 3 offers, matching 2.1.5.
pub const TRANSCODE_TARGETS: &[TranscodeTarget] = &[
    TranscodeTarget {
        name: "MP3",
        extension: "mp3",
    },
    TranscodeTarget {
        name: "AAC",
        extension: "m4a",
    },
    TranscodeTarget {
        name: "FLAC",
        extension: "flac",
    },
    TranscodeTarget {
        name: "Ogg Vorbis",
        extension: "ogg",
    },
    TranscodeTarget {
        name: "Opus",
        extension: "opus",
    },
    TranscodeTarget {
        name: "Speex",
        extension: "spx",
    },
    TranscodeTarget {
        name: "WavPack",
        extension: "wv",
    },
    TranscodeTarget {
        name: "ASF",
        extension: "wma",
    },
];

/// Identify a codec by file extension (case-insensitive, dot optional).
pub fn codec_for_extension(ext: &str) -> Option<&'static Codec> {
    let clean = ext.trim_start_matches('.').to_ascii_lowercase();
    SUPPORTED_CODECS
        .iter()
        .find(|c| c.extensions.contains(&clean.as_str()))
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn all_2_1_5_codecs_present() {
        let names: Vec<_> = SUPPORTED_CODECS.iter().map(|c| c.name).collect();
        for required in [
            "WAV",
            "FLAC",
            "Ogg FLAC",
            "WavPack",
            "Ogg Vorbis",
            "Opus",
            "Ogg Speex",
            "MPC",
            "TrueAudio",
            "AIFF",
            "MP4/AAC",
            "ALAC",
            "MP3",
            "ASF",
            "Monkey's Audio",
            "DSD (DSF)",
            "DSD (DSDIFF)",
        ] {
            assert!(names.contains(&required), "missing codec {required}");
        }
    }

    #[test]
    fn all_transcode_targets_present() {
        let names: Vec<_> = TRANSCODE_TARGETS.iter().map(|t| t.name).collect();
        for required in [
            "MP3",
            "AAC",
            "FLAC",
            "Ogg Vorbis",
            "Opus",
            "Speex",
            "WavPack",
            "ASF",
        ] {
            assert!(names.contains(&required), "missing target {required}");
        }
    }

    #[test]
    fn extension_lookup() {
        assert_eq!(codec_for_extension("flac").unwrap().name, "FLAC");
        assert_eq!(codec_for_extension(".MP3").unwrap().name, "MP3");
        assert_eq!(codec_for_extension("dsf").unwrap().name, "DSD (DSF)");
        assert!(codec_for_extension("exe").is_none());
    }
}
