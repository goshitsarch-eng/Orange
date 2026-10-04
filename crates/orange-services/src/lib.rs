//! UI-independent Orange services shared by the Flutter frontend and reference CLI.
pub mod commands;
pub mod files;
pub mod library;
#[cfg(all(feature = "dbus", target_os = "linux"))]
pub mod mpris_host;
#[cfg(feature = "notify")]
pub mod notify;
pub mod service;
pub mod settings;
pub mod state;
