//! Live GStreamer backend proofs (feature `gst`).
//!
//! These run real pipelines against the system plugin set — no audio
//! hardware needed (test tones sink to `fakesink`; transcodes write to the
//! temp dir). They fail loudly when an expected element is missing, which is
//! exactly the Flatpak `check-gstreamer` gate in test form.

#![cfg(feature = "gst")]

use std::time::Duration;

use orange_media::backend::{AudioSink, FxChain, PlaybackChain, TranscodeChain};
use orange_media::backend_gst::{probe_uri, render_test_wav, transcode_file, GstEngine};
use orange_media::playback::EngineState;

fn temp_file(name: &str) -> String {
    let mut path = std::env::temp_dir();
    path.push(format!("orange-gst-{name}-{}", std::process::id()));
    path.to_string_lossy().into_owned()
}

fn wait_for_state(engine: &GstEngine, want: EngineState) {
    let deadline = std::time::Instant::now() + Duration::from_secs(10);
    while std::time::Instant::now() < deadline {
        if let Some(event) = engine.poll_event(Duration::from_millis(200)) {
            if event == orange_media::backend_gst::EngineEvent::State(want) {
                return;
            }
            if matches!(event, orange_media::backend_gst::EngineEvent::Error(_)) {
                panic!("pipeline error before {want:?}: {event:?}");
            }
        }
    }
    panic!("timed out waiting for {want:?}");
}

#[test]
fn tone_pipeline_plays_and_reaches_eos() {
    let engine = GstEngine::for_test_tone(&FxChain::default(), 50).unwrap();
    engine.play().unwrap();
    wait_for_state(&engine, EngineState::Playing);
    let event = engine.run_until_terminal(Duration::from_secs(15)).unwrap();
    assert_eq!(event, orange_media::backend_gst::EngineEvent::Eos);
    engine.stop().unwrap();
}

#[test]
fn fx_chain_inserts_real_elements_and_spectrum_flows() {
    let fx = FxChain {
        equalizer_db: Some([0.0; 10]),
        replaygain_preamp_db: None,
        spectrum_bands: Some(16),
    };
    let engine = GstEngine::for_test_tone(&fx, 400).unwrap();
    assert!(engine.has_element("orange-eq"));
    assert!(engine.has_element("orange-spectrum"));
    assert!(!engine.has_element("orange-rgvolume"));
    assert!(!engine.has_element("orange-volume"));
    engine.play().unwrap();
    let deadline = std::time::Instant::now() + Duration::from_secs(15);
    let mut ticks = 0;
    while std::time::Instant::now() < deadline && ticks == 0 {
        if let Some(orange_media::backend_gst::EngineEvent::SpectrumTick { magnitudes_db }) =
            engine.poll_event(Duration::from_millis(200))
        {
            assert_eq!(magnitudes_db.len(), 16);
            ticks += 1;
        }
    }
    engine.stop().unwrap();
    assert!(ticks > 0, "no spectrum messages arrived");
}

#[test]
fn transcode_wav_to_flac_and_mp3() {
    let source = temp_file("src.wav");
    render_test_wav(&source, 1).unwrap();
    let source_uri = format!("file://{source}");

    for target in ["FLAC", "MP3"] {
        let output = temp_file(&format!("out-{target}"));
        let chain = TranscodeChain {
            input_uri: source_uri.clone(),
            target_name: target.to_string(),
            output_path: output.clone(),
        };
        let report = transcode_file(&chain, Duration::from_secs(60)).unwrap();
        assert_eq!(report.output_path, output);
        assert!(
            report.bytes_written > 1000,
            "suspiciously small {target} output"
        );
        std::fs::remove_file(&output).ok();
    }
    std::fs::remove_file(&source).ok();
}

#[test]
fn probe_reports_duration_and_audio() {
    let source = temp_file("probe.wav");
    render_test_wav(&source, 1).unwrap();
    let report = probe_uri(&format!("file://{source}"), Duration::from_secs(15)).unwrap();
    assert!(report.saw_audio_pad);
    let nanos = report.duration_nanos.expect("probe found no duration");
    let secs = nanos as f64 / 1_000_000_000.0;
    assert!(
        (0.5..2.0).contains(&secs),
        "unexpected probed duration: {secs}s"
    );
    std::fs::remove_file(&source).ok();
}

#[test]
fn cdda_source_builds_without_drive() {
    // Element construction needs no hardware; only playback would.
    let chain = PlaybackChain {
        uri: "cdda:///dev/sr0/1".to_string(),
        sink: AudioSink::Auto,
        fx: FxChain::default(),
    };
    let engine = GstEngine::new_playback(
        &chain,
        None,
        false,
        orange_media::audio_fx::NormalizationMode::Off,
    )
    .unwrap();
    assert!(!engine.has_element("orange-volume"));
    engine.stop().unwrap();
}

#[test]
fn playback_builder_accepts_device_sink() {
    // Structural only: device-addressed sinks construct without a device.
    let chain = PlaybackChain {
        uri: "file:///music/a.flac".to_string(),
        sink: AudioSink::Alsa {
            device: Some("hw:0,0".to_string()),
        },
        fx: FxChain::default(),
    };
    let engine = GstEngine::new_playback(
        &chain,
        None,
        false,
        orange_media::audio_fx::NormalizationMode::Off,
    )
    .unwrap();
    assert!(!engine.has_element("orange-volume"));
    engine.stop().unwrap();
}
