//! Playback/transcode pipeline descriptions (no GStreamer dependency).
//!
//! Bit-perfect contract, mirroring the 2.1.5 engine: the audio path is
//! `uridecodebin ! audioconvert ! audioresample ! sink`, and `audioconvert`
//! / `audioresample` are bit-transparent when the sink accepts the stream
//! format. No software volume, equalizer, normalization, or resampling
//! touches the stream unless the user enables it — enabling FX appends real
//! elements (see [`PlaybackChain`]). The `gst` feature builds these
//! descriptions into live pipelines in [`crate::backend_gst`].

/// Audio sink selection. `Auto` lets GStreamer pick (PulseAudio on a
/// desktop session, ALSA on bare metal), exactly like 2.1.5 defaults.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub enum AudioSink {
    #[default]
    Auto,
    Pulse {
        device: Option<String>,
    },
    Alsa {
        device: Option<String>,
    },
}

impl AudioSink {
    /// Element description for the sink stage.
    pub fn element(&self) -> String {
        match self {
            Self::Auto => "autoaudiosink".to_string(),
            Self::Pulse { device: None } => "pulsesink".to_string(),
            Self::Pulse { device: Some(d) } => format!("pulsesink device={}", quote(d)),
            Self::Alsa { device: None } => "alsasink".to_string(),
            Self::Alsa { device: Some(d) } => format!("alsasink device={}", quote(d)),
        }
    }
}

/// Encoder + optional muxer for one transcode target, verified against the
/// SDK/host plugin set (see the Flatpak `check-gstreamer` module).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct EncoderChain {
    pub encoder: &'static str,
    pub muxer: Option<&'static str>,
}

/// Encoder chain by 2.1.5 transcode target name.
pub fn encoder_chain(target_name: &str) -> Option<EncoderChain> {
    match target_name.trim().to_ascii_lowercase().as_str() {
        "mp3" => Some(EncoderChain {
            encoder: "lamemp3enc",
            muxer: None,
        }),
        // fdkaacenc preferred over avenc_aac; both verified present.
        "aac" => Some(EncoderChain {
            encoder: "fdkaacenc",
            muxer: Some("mp4mux"),
        }),
        "flac" => Some(EncoderChain {
            encoder: "flacenc",
            muxer: None,
        }),
        "ogg vorbis" => Some(EncoderChain {
            encoder: "vorbisenc",
            muxer: Some("oggmux"),
        }),
        "opus" => Some(EncoderChain {
            encoder: "opusenc",
            muxer: Some("oggmux"),
        }),
        "speex" => Some(EncoderChain {
            encoder: "speexenc",
            muxer: Some("oggmux"),
        }),
        "wavpack" => Some(EncoderChain {
            encoder: "wavpackenc",
            muxer: None,
        }),
        "asf" => Some(EncoderChain {
            encoder: "avenc_wmav2",
            muxer: Some("avmux_asf"),
        }),
        _ => None,
    }
}

/// Optional DSP stages. `None` everywhere means the bit-perfect path.
#[derive(Debug, Clone, Default, PartialEq)]
pub struct FxChain {
    /// 10 band gains in dB; `None` omits the equalizer element entirely.
    pub equalizer_db: Option<[f64; 10]>,
    /// ReplayGain pre-amp in dB; `None` omits the `rgvolume` element.
    pub replaygain_preamp_db: Option<f64>,
    /// Spectrum analyzer bands; `None` omits the `spectrum` element.
    pub spectrum_bands: Option<u32>,
}

/// A playback pipeline description.
#[derive(Debug, Clone, PartialEq)]
pub struct PlaybackChain {
    pub uri: String,
    pub sink: AudioSink,
    pub fx: FxChain,
}

impl PlaybackChain {
    /// gst-launch-style description. With default FX this is exactly
    /// decode → convert → resample → sink (bit-perfect shape).
    pub fn describe(&self) -> Option<String> {
        let uri = sanitize_uri(&self.uri)?;
        let mut stages = vec![
            format!("uridecodebin uri={uri}"),
            "audioconvert".to_string(),
            "audioresample".to_string(),
        ];
        if let Some(gains) = &self.fx.equalizer_db {
            let props: Vec<String> = gains
                .iter()
                .enumerate()
                .map(|(band, gain)| format!("band{band}={gain:.1}"))
                .collect();
            stages.push(format!("equalizer-10bands {}", props.join(" ")));
        }
        if let Some(preamp) = self.fx.replaygain_preamp_db {
            stages.push(format!("rgvolume pre-amp={preamp:.1}"));
        }
        if let Some(bands) = self.fx.spectrum_bands {
            stages.push(format!("spectrum bands={bands}"));
        }
        stages.push(self.sink.element());
        Some(stages.join(" ! "))
    }

    /// True when the chain carries no DSP: the bit-perfect path.
    pub fn is_bit_perfect(&self) -> bool {
        self.fx == FxChain::default()
    }
}

/// A transcode pipeline description.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TranscodeChain {
    pub input_uri: String,
    pub target_name: String,
    pub output_path: String,
}

impl TranscodeChain {
    /// gst-launch-style description, or `None` for unknown targets and
    /// unsafe (quote-bearing) paths.
    pub fn describe(&self) -> Option<String> {
        let chain = encoder_chain(&self.target_name)?;
        let uri = sanitize_uri(&self.input_uri)?;
        if self.output_path.contains('"') {
            return None;
        }
        let mut stages = vec![
            format!("uridecodebin uri={uri}"),
            "audioconvert".to_string(),
            "audioresample".to_string(),
            chain.encoder.to_string(),
        ];
        if let Some(muxer) = chain.muxer {
            stages.push(muxer.to_string());
        }
        stages.push(format!("filesink location=\"{}\"", self.output_path));
        Some(stages.join(" ! "))
    }
}

fn quote(value: &str) -> String {
    format!("\"{}\"", value.replace('"', ""))
}

/// URIs with double quotes break `gst_parse_launch` quoting; refuse them.
fn sanitize_uri(uri: &str) -> Option<String> {
    if uri.is_empty() || uri.contains('"') {
        return None;
    }
    Some(format!("\"{uri}\""))
}

#[cfg(test)]
mod tests {
    use super::*;
    use orange_core::codecs::TRANSCODE_TARGETS;

    #[test]
    fn every_target_has_an_encoder_chain() {
        for target in TRANSCODE_TARGETS {
            assert!(
                encoder_chain(target.name).is_some(),
                "no encoder chain for {}",
                target.name
            );
        }
        assert!(encoder_chain("bogus").is_none());
    }

    #[test]
    fn transcode_description_shape() {
        let chain = TranscodeChain {
            input_uri: "file:///music/a.flac".to_string(),
            target_name: "Ogg Vorbis".to_string(),
            output_path: "/tmp/a.ogg".to_string(),
        };
        assert_eq!(
            chain.describe().unwrap(),
            "uridecodebin uri=\"file:///music/a.flac\" ! audioconvert ! \
             audioresample ! vorbisenc ! oggmux ! filesink location=\"/tmp/a.ogg\""
        );
        let raw = TranscodeChain {
            target_name: "MP3".to_string(),
            ..chain.clone()
        };
        assert!(raw.describe().unwrap().contains("lamemp3enc ! filesink"));
        assert!(TranscodeChain {
            target_name: "Nope".to_string(),
            ..chain.clone()
        }
        .describe()
        .is_none());
    }

    #[test]
    fn unsafe_paths_refused() {
        let chain = TranscodeChain {
            input_uri: "file:///a\".flac".to_string(),
            target_name: "MP3".to_string(),
            output_path: "/tmp/a.mp3".to_string(),
        };
        assert!(chain.describe().is_none());
    }

    #[test]
    fn default_playback_is_bit_perfect() {
        let chain = PlaybackChain {
            uri: "file:///music/a.flac".to_string(),
            sink: AudioSink::Auto,
            fx: FxChain::default(),
        };
        assert!(chain.is_bit_perfect());
        assert_eq!(
            chain.describe().unwrap(),
            "uridecodebin uri=\"file:///music/a.flac\" ! audioconvert ! \
             audioresample ! autoaudiosink"
        );
    }

    #[test]
    fn fx_chain_adds_real_elements() {
        let chain = PlaybackChain {
            uri: "file:///music/a.flac".to_string(),
            sink: AudioSink::Pulse { device: None },
            fx: FxChain {
                equalizer_db: Some([0.0; 10]),
                replaygain_preamp_db: Some(-3.0),
                spectrum_bands: Some(48),
            },
        };
        assert!(!chain.is_bit_perfect());
        let desc = chain.describe().unwrap();
        assert!(desc.contains("equalizer-10bands band0=0.0"));
        assert!(desc.contains("rgvolume pre-amp=-3.0"));
        assert!(desc.contains("spectrum bands=48"));
        assert!(desc.contains("pulsesink"));
    }
}
