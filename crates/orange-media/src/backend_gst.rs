//! Live GStreamer engine (feature `gst`).
//!
//! Builds the pipelines described in [`crate::backend`] with `gstreamer-rs`:
//! manual `uridecodebin`-rooted chains (never `playbin`), so the only
//! elements in the bit-perfect path are decode → convert → resample → sink.
//! Video and text are excluded by construction (no video/text sinks exist),
//! which is the audio-only guarantee without fragile playbin flag bits.
//!
//! The engine reports bus events into the UI-agnostic [`crate::playback`]
//! state machine and the MPRIS layer; it never touches Qt.

use std::time::{Duration, Instant};

use gst::prelude::*;
use gstreamer as gst;

use crate::audio_fx::NormalizationMode;
use crate::backend::{encoder_chain, AudioSink, FxChain, PlaybackChain, TranscodeChain};
use crate::playback::EngineState;

/// Engine failure.
#[derive(Debug)]
pub struct GstError(pub String);

impl std::fmt::Display for GstError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "gstreamer: {}", self.0)
    }
}

impl std::error::Error for GstError {}

impl From<gst::glib::BoolError> for GstError {
    fn from(e: gst::glib::BoolError) -> Self {
        Self(e.to_string())
    }
}

impl From<gst::StateChangeError> for GstError {
    fn from(e: gst::StateChangeError) -> Self {
        Self(e.to_string())
    }
}

/// One bus event, already mapped to Orange state.
#[derive(Debug, Clone, PartialEq)]
pub enum EngineEvent {
    Eos,
    Error(String),
    State(EngineState),
    SpectrumTick { magnitudes_db: Vec<f32> },
}

/// Live playback pipeline.
pub struct GstEngine {
    pipeline: gst::Pipeline,
    bus: gst::Bus,
}

impl GstEngine {
    /// Build a URI playback pipeline. `software_volume` (0.0-1.0) inserts a
    /// `volume` element; `None` keeps the bit-perfect path element-free.
    /// `replaygain_album` selects rgvolume album vs track mode.
    pub fn new_playback(
        chain: &PlaybackChain,
        software_volume: Option<f64>,
        replaygain_album: bool,
        replaygain_mode: NormalizationMode,
    ) -> Result<Self, GstError> {
        gst::init().map_err(|e| GstError(e.to_string()))?;
        let (src, dynamic) = make_source(&chain.uri)?;
        let sink = make_audio_sink(&chain.sink)?;
        Self::build(
            src,
            dynamic,
            &chain.fx,
            sink,
            software_volume,
            replaygain_album,
            replaygain_mode,
        )
    }

    /// Test-tone pipeline with the same chain shape, sinking to `fakesink`.
    /// Used by the test suite so no audio files or hardware are needed.
    pub fn for_test_tone(chain_fx: &FxChain, buffers: u32) -> Result<Self, GstError> {
        gst::init().map_err(|e| GstError(e.to_string()))?;
        // Defaults are already sine at 440 Hz; only bound the buffer count.
        let tone = gst::ElementFactory::make("audiotestsrc")
            .property("num-buffers", buffers as i32)
            .build()?;
        let sink = gst::ElementFactory::make("fakesink").build()?;
        Self::build(
            tone,
            false,
            chain_fx,
            sink,
            None,
            false,
            NormalizationMode::Off,
        )
    }

    /// Assemble `source [! uridecodebin pad wiring] ! conv ! resample !
    /// [fx...] ! sink`. `dynamic_source` links decode pads on `pad-added`;
    /// static sources link directly.
    fn build(
        source: gst::Element,
        dynamic_source: bool,
        fx: &FxChain,
        sink: gst::Element,
        software_volume: Option<f64>,
        replaygain_album: bool,
        replaygain_mode: NormalizationMode,
    ) -> Result<Self, GstError> {
        let pipeline = gst::Pipeline::new();
        let conv = gst::ElementFactory::make("audioconvert").build()?;
        let resample = gst::ElementFactory::make("audioresample").build()?;
        pipeline.add_many([&source, &conv, &resample])?;

        let mut head = resample.clone();
        let mut named: Vec<gst::Element> = Vec::new();
        if fx.equalizer_db.is_some() {
            named.push(
                gst::ElementFactory::make("equalizer-10bands")
                    .name("orange-eq")
                    .build()?,
            );
        }
        if fx.replaygain_preamp_db.is_some() && replaygain_mode != NormalizationMode::Off {
            named.push(
                gst::ElementFactory::make("rgvolume")
                    .name("orange-rgvolume")
                    .build()?,
            );
        }
        if fx.spectrum_bands.is_some() {
            named.push(
                gst::ElementFactory::make("spectrum")
                    .name("orange-spectrum")
                    .build()?,
            );
        }
        if software_volume.is_some() {
            named.push(
                gst::ElementFactory::make("volume")
                    .name("orange-volume")
                    .build()?,
            );
        }
        for element in &named {
            pipeline.add(element)?;
        }
        for element in &named {
            head.link(element)?;
            head = element.clone();
        }
        pipeline.add(&sink)?;
        head.link(&sink)?;

        if dynamic_source {
            // uridecodebin exposes decoded pads dynamically; link audio pads.
            let conv_weak = conv.downgrade();
            source.connect_pad_added(move |_src, pad| {
                link_audio_pad(pad, &conv_weak);
            });
        } else {
            source.link(&conv)?;
        }

        conv.link(&resample)?;
        let bus = pipeline.bus().expect("pipeline bus");
        let engine = Self { pipeline, bus };
        engine.apply_fx(fx, replaygain_album)?;
        if let Some(level) = software_volume {
            engine.set_software_volume(level)?;
        }
        Ok(engine)
    }

    fn apply_fx(&self, fx: &FxChain, replaygain_album: bool) -> Result<(), GstError> {
        if let Some(gains) = fx.equalizer_db {
            let eq = self
                .pipeline
                .by_name("orange-eq")
                .ok_or_else(|| GstError("equalizer element missing".to_string()))?;
            for (band, gain) in gains.iter().enumerate() {
                eq.set_property(&format!("band{band}"), gain.clamp(-12.0, 12.0));
            }
        }
        if let Some(preamp) = fx.replaygain_preamp_db {
            if let Some(rg) = self.pipeline.by_name("orange-rgvolume") {
                rg.set_property("pre-amp", preamp);
                rg.set_property("album-mode", replaygain_album);
            }
        }
        if let Some(bands) = fx.spectrum_bands {
            if let Some(spectrum) = self.pipeline.by_name("orange-spectrum") {
                spectrum.set_property("bands", bands);
                spectrum.set_property("post-messages", true);
                spectrum.set_property("interval", 50_000_000u64);
            }
        }
        Ok(())
    }

    fn set_software_volume(&self, level: f64) -> Result<(), GstError> {
        let vol = self
            .pipeline
            .by_name("orange-volume")
            .ok_or_else(|| GstError("volume element missing".to_string()))?;
        vol.set_property("volume", level.clamp(0.0, 1.0));
        Ok(())
    }

    /// Live software volume 0.0–1.0. The volume element is always in the
    /// UI playback chain so the header slider can move without a rebuild.
    pub fn set_output_volume(&self, level: f64) -> Result<(), GstError> {
        self.set_software_volume(level)
    }

    pub fn play(&self) -> Result<(), GstError> {
        self.pipeline.set_state(gst::State::Playing)?;
        Ok(())
    }

    pub fn pause(&self) -> Result<(), GstError> {
        self.pipeline.set_state(gst::State::Paused)?;
        Ok(())
    }

    pub fn stop(&self) -> Result<(), GstError> {
        self.pipeline.set_state(gst::State::Null)?;
        Ok(())
    }

    /// Seek to whole seconds.
    pub fn seek_secs(&self, secs: u64) -> Result<(), GstError> {
        self.pipeline.seek_simple(
            gst::SeekFlags::FLUSH | gst::SeekFlags::KEY_UNIT,
            gst::ClockTime::from_seconds(secs),
        )?;
        Ok(())
    }

    /// Position in nanoseconds, if known.
    pub fn position_nanos(&self) -> Option<u64> {
        self.pipeline
            .query_position::<gst::ClockTime>()
            .map(|time| time.nseconds())
    }

    /// Duration in nanoseconds, if known.
    pub fn duration_nanos(&self) -> Option<u64> {
        self.pipeline
            .query_duration::<gst::ClockTime>()
            .map(|time| time.nseconds())
    }

    /// True when a named element exists in the pipeline (tests, diagnostics).
    pub fn has_element(&self, name: &str) -> bool {
        self.pipeline.by_name(name).is_some()
    }

    /// Next bus event within `timeout`.
    pub fn poll_event(&self, timeout: Duration) -> Option<EngineEvent> {
        let msg = self.bus.timed_pop(clock_timeout(timeout))?;
        match msg.view() {
            gst::MessageView::Eos(..) => Some(EngineEvent::Eos),
            gst::MessageView::Error(err) => Some(EngineEvent::Error(format!(
                "{} ({:?})",
                err.error(),
                err.debug()
            ))),
            gst::MessageView::StateChanged(state) => {
                if state.src().is_some_and(|src| src == &self.pipeline) {
                    Some(EngineEvent::State(map_state(state.current())))
                } else {
                    None
                }
            }
            gst::MessageView::Element(element) => {
                let structure = element.structure()?;
                if structure.name() != "spectrum" {
                    return None;
                }
                // Newer GStreamer posts magnitudes as GstValueList.
                let list = structure.get::<gst::List>("magnitude").ok()?;
                let magnitudes: Vec<f32> = list
                    .as_slice()
                    .iter()
                    .filter_map(|value| value.get::<f32>().ok())
                    .collect();
                Some(EngineEvent::SpectrumTick {
                    magnitudes_db: magnitudes,
                })
            }
            _ => None,
        }
    }

}

impl Drop for GstEngine {
    fn drop(&mut self) {
        let _ = self.pipeline.set_state(gst::State::Null);
    }
}

impl GstEngine {
    /// Run until EOS or error. Returns on the first terminal event.
    pub fn run_until_terminal(&self, timeout: Duration) -> Result<EngineEvent, GstError> {
        let deadline = Instant::now() + timeout;
        loop {
            let remaining = deadline.saturating_duration_since(Instant::now());
            if remaining.is_zero() {
                return Err(GstError("timed out waiting for EOS".to_string()));
            }
            match self.poll_event(remaining) {
                Some(event @ (EngineEvent::Eos | EngineEvent::Error(_))) => return Ok(event),
                Some(_) => continue,
                None => continue,
            }
        }
    }
}

/// Bus timeouts take [`gst::ClockTime`]; convert a [`Duration`].
fn clock_timeout(timeout: Duration) -> gst::ClockTime {
    gst::ClockTime::from_nseconds(timeout.as_nanos().min(u64::MAX as u128) as u64)
}

/// Link a newly appeared source pad to `audioconvert` when it carries audio.
fn link_audio_pad(pad: &gst::Pad, conv: &gst::glib::WeakRef<gst::Element>) {
    let Some(conv) = conv.upgrade() else {
        return;
    };
    let caps = pad.current_caps().or_else(|| Some(pad.query_caps(None)));
    let is_audio = caps
        .map(|caps| {
            caps.iter()
                .any(|structure| structure.name().starts_with("audio/"))
        })
        .unwrap_or(false);
    if !is_audio {
        return;
    }
    let sink_pad = conv.static_pad("sink").expect("audioconvert sink pad");
    if !sink_pad.is_linked() {
        let _ = pad.link(&sink_pad);
    }
}

/// Build the source stage: `cdiocddasrc` for `cdda://` URIs (Audio CD),
/// `uridecodebin` for everything else. Returns the element plus whether its
/// pads appear dynamically.
fn make_source(uri: &str) -> Result<(gst::Element, bool), GstError> {
    if let Some((device, track)) = crate::cd::parse_cdda_url(uri) {
        let src = gst::ElementFactory::make("cdiocddasrc")
            .property("device", device.as_str())
            .property("track", track)
            .build()?;
        return Ok((src, false));
    }
    let src = gst::ElementFactory::make("uridecodebin")
        .property("uri", uri)
        .build()?;
    Ok((src, true))
}

fn map_state(state: gst::State) -> EngineState {
    match state {
        gst::State::Playing => EngineState::Playing,
        gst::State::Paused => EngineState::Paused,
        _ => EngineState::Idle,
    }
}

fn make_audio_sink(sink: &AudioSink) -> Result<gst::Element, GstError> {
    let element = match sink {
        AudioSink::Auto => gst::ElementFactory::make("autoaudiosink").build()?,
        AudioSink::Pulse { device } => {
            let builder = gst::ElementFactory::make("pulsesink");
            match device {
                Some(device) => builder.property("device", device.as_str()).build()?,
                None => builder.build()?,
            }
        }
        AudioSink::Alsa { device } => {
            let builder = gst::ElementFactory::make("alsasink");
            match device {
                Some(device) => builder.property("device", device.as_str()).build()?,
                None => builder.build()?,
            }
        }
    };
    Ok(element)
}

/// Transcode report.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TranscodeReport {
    pub output_path: String,
    pub bytes_written: u64,
}

/// Transcode `chain` to completion. The pipeline is built from the same
/// encoder table as [`crate::backend::encoder_chain`], so the pure
/// description and the live run cannot drift apart.
pub fn transcode_file(
    chain: &TranscodeChain,
    timeout: Duration,
) -> Result<TranscodeReport, GstError> {
    gst::init().map_err(|e| GstError(e.to_string()))?;
    let table = encoder_chain(&chain.target_name)
        .ok_or_else(|| GstError(format!("unknown transcode target: {}", chain.target_name)))?;
    if chain.input_uri.is_empty()
        || chain.input_uri.contains('"')
        || chain.output_path.contains('"')
    {
        return Err(GstError("unsafe transcode path".to_string()));
    }
    let pipeline = gst::Pipeline::new();
    let (src, dynamic) = make_source(&chain.input_uri)?;
    let conv = gst::ElementFactory::make("audioconvert").build()?;
    let resample = gst::ElementFactory::make("audioresample").build()?;
    let encoder = gst::ElementFactory::make(table.encoder).build()?;
    pipeline.add_many([&src, &conv, &resample, &encoder])?;
    conv.link(&resample)?;
    resample.link(&encoder)?;
    let tail = if let Some(muxer) = table.muxer {
        let mux = gst::ElementFactory::make(muxer).build()?;
        pipeline.add(&mux)?;
        encoder.link(&mux)?;
        mux
    } else {
        encoder.clone()
    };
    let sink = gst::ElementFactory::make("filesink")
        .property("location", chain.output_path.as_str())
        .build()?;
    pipeline.add(&sink)?;
    tail.link(&sink)?;

    if dynamic {
        let conv_weak = conv.downgrade();
        src.connect_pad_added(move |_src, pad| {
            link_audio_pad(pad, &conv_weak);
        });
    } else {
        src.link(&conv)?;
    }

    pipeline.set_state(gst::State::Playing)?;
    let bus = pipeline.bus().expect("pipeline bus");
    let deadline = Instant::now() + timeout;
    let result = loop {
        let remaining = deadline.saturating_duration_since(Instant::now());
        if remaining.is_zero() {
            break Err(GstError("transcode timed out".to_string()));
        }
        let Some(msg) = bus.timed_pop(clock_timeout(remaining)) else {
            continue;
        };
        match msg.view() {
            gst::MessageView::Eos(..) => break Ok(()),
            gst::MessageView::Error(err) => {
                break Err(GstError(format!("transcode failed: {}", err.error())));
            }
            _ => continue,
        }
    };
    let _ = pipeline.set_state(gst::State::Null);
    result?;
    let bytes_written = std::fs::metadata(&chain.output_path)
        .map_err(|e| GstError(format!("transcode output missing: {e}")))?
        .len();
    Ok(TranscodeReport {
        output_path: chain.output_path.clone(),
        bytes_written,
    })
}

/// Render `secs` seconds of sine tone to a WAV file. Test helper shared by
/// the tagger suite so tag read/write tests need no fixture audio.
pub fn render_test_wav(path: &str, secs: u32) -> Result<(), GstError> {
    gst::init().map_err(|e| GstError(e.to_string()))?;
    if path.is_empty() || path.contains('"') {
        return Err(GstError("unsafe render path".to_string()));
    }
    let pipeline = gst::Pipeline::new();
    // Defaults are already sine at 440 Hz; 1024 samples per buffer at
    // 44.1 kHz, so this many buffers make roughly `secs` seconds.
    let src = gst::ElementFactory::make("audiotestsrc")
        .property("num-buffers", ((secs * 44_100 + 1023) / 1024) as i32)
        .build()?;
    let enc = gst::ElementFactory::make("wavenc").build()?;
    let sink = gst::ElementFactory::make("filesink")
        .property("location", path)
        .build()?;
    pipeline.add_many([&src, &enc, &sink])?;
    src.link(&enc)?;
    enc.link(&sink)?;
    pipeline.set_state(gst::State::Playing)?;
    let bus = pipeline.bus().expect("pipeline bus");
    let deadline = Instant::now() + Duration::from_secs(30);
    loop {
        let remaining = deadline.saturating_duration_since(Instant::now());
        if remaining.is_zero() {
            let _ = pipeline.set_state(gst::State::Null);
            return Err(GstError("render timed out".to_string()));
        }
        let Some(msg) = bus.timed_pop(clock_timeout(remaining)) else {
            continue;
        };
        match msg.view() {
            gst::MessageView::Eos(..) => break,
            gst::MessageView::Error(err) => {
                let _ = pipeline.set_state(gst::State::Null);
                return Err(GstError(format!("render failed: {}", err.error())));
            }
            _ => continue,
        }
    }
    let _ = pipeline.set_state(gst::State::Null);
    Ok(())
}

/// Probe result: can GStreamer play this URI, and how long is it?
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct ProbeReport {
    pub uri: String,
    pub duration_nanos: Option<u64>,
    pub saw_audio_pad: bool,
}

/// Preroll `uri` (no audible output) and report duration.
pub fn probe_uri(uri: &str, timeout: Duration) -> Result<ProbeReport, GstError> {
    gst::init().map_err(|e| GstError(e.to_string()))?;
    if uri.is_empty() || uri.contains('"') {
        return Err(GstError("unsafe probe URI".to_string()));
    }
    let pipeline = gst::Pipeline::new();
    let (src, dynamic) = make_source(uri)?;
    let sink = gst::ElementFactory::make("fakesink").build()?;
    pipeline.add_many([&src, &sink])?;
    let audio_seen = std::sync::Arc::new(std::sync::atomic::AtomicBool::new(false));
    if dynamic {
        let sink_weak = sink.downgrade();
        let audio_seen_probe = audio_seen.clone();
        src.connect_pad_added(move |_src, pad| {
            let Some(sink) = sink_weak.upgrade() else {
                return;
            };
            let caps = pad.current_caps().or_else(|| Some(pad.query_caps(None)));
            let is_audio = caps
                .map(|caps| {
                    caps.iter()
                        .any(|structure| structure.name().starts_with("audio/"))
                })
                .unwrap_or(false);
            if !is_audio {
                return;
            }
            audio_seen_probe.store(true, std::sync::atomic::Ordering::SeqCst);
            let sink_pad = sink.static_pad("sink").expect("fakesink sink pad");
            if !sink_pad.is_linked() {
                let _ = pad.link(&sink_pad);
            }
        });
    } else {
        // Static CD source: audio by construction.
        audio_seen.store(true, std::sync::atomic::Ordering::SeqCst);
        src.link(&sink)?;
    }
    pipeline.set_state(gst::State::Paused)?;
    let bus = pipeline.bus().expect("pipeline bus");
    let deadline = Instant::now() + timeout;
    let mut duration_nanos = None;
    loop {
        let remaining = deadline.saturating_duration_since(Instant::now());
        if remaining.is_zero() {
            break;
        }
        let Some(msg) = bus.timed_pop(clock_timeout(remaining)) else {
            continue;
        };
        match msg.view() {
            gst::MessageView::AsyncDone(..) => {
                duration_nanos = pipeline
                    .query_duration::<gst::ClockTime>()
                    .map(|time| time.nseconds());
                break;
            }
            gst::MessageView::Error(err) => {
                let _ = pipeline.set_state(gst::State::Null);
                return Err(GstError(format!("probe failed: {}", err.error())));
            }
            _ => continue,
        }
    }
    let _ = pipeline.set_state(gst::State::Null);
    Ok(ProbeReport {
        uri: uri.to_string(),
        duration_nanos,
        saw_audio_pad: audio_seen.load(std::sync::atomic::Ordering::SeqCst),
    })
}
