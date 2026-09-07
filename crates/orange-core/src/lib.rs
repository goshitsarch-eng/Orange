//! Orange 3.0 core: catalog identity, version/branding, codec tables,
//! song model mirroring the 2.1.5 `songs` table, and appearance mode.
//! Zero external dependencies. Zero telemetry.

pub mod appearance;
pub mod codecs;
pub mod identity;
pub mod paths;
pub mod song;
pub mod version;
