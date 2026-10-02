//! Orange desktop application, typed commands and background services.
//! The shared Dioxus UI lives in [`ui`] with optional native integrations.

pub mod about;
pub mod commands;
pub mod dialogs;
pub mod files;
pub mod library;
#[cfg(all(feature = "dbus", target_os = "linux"))]
pub mod mpris_host;
pub mod nav;
#[cfg(feature = "notify")]
pub mod notify;
pub mod playerbar;
pub mod service;
pub mod settings;
pub mod state;

#[cfg(feature = "desktop")]
pub mod platform;
#[cfg(feature = "desktop")]
pub mod ui;

pub use orange_core::identity::APP_ID;
