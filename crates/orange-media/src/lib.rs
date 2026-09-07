//! Media layer: playback state machine, audio FX (equalizer, ReplayGain,
//! EBU R128, analyzer, moodbar, waveform), radio, online services
//! (scrobbling, covers, lyrics, streaming), devices/CD/transcoding, MPRIS.
//!
//! The `gst` feature wires the real GStreamer pipeline; without it this
//! crate is the pure state/type layer used by tests and the headless shell.
//! Zero telemetry. Streaming credentials are never logged (see `online`).

pub mod audio_fx;
pub mod backend;
#[cfg(feature = "gst")]
pub mod backend_gst;
pub mod cd;
pub mod devices;
pub mod discord;
pub mod mpris;
#[cfg(feature = "dbus")]
pub mod mpris_client;
#[cfg(feature = "dbus")]
pub mod mpris_server;
#[cfg(feature = "online")]
pub mod net;
pub mod online;
pub mod playback;
pub mod radio;
pub mod scrobble;
#[cfg(feature = "tags")]
pub mod tagger;
#[cfg(feature = "dbus")]
pub mod udisks;
