//! Orange 3.0 shell: navigation, player bar layout, dialogs, About.
//! The native COSMIC window lives in [`ui`] (feature `ui`); everything else
//! here is toolkit-independent so it is tested on every `cargo test`.

pub mod about;
pub mod dialogs;
#[cfg(feature = "dbus")]
pub mod mpris_host;
pub mod nav;
#[cfg(feature = "notify")]
pub mod notify;
pub mod playerbar;

#[cfg(feature = "ui")]
pub mod ui;

pub use orange_core::identity::APP_ID;
