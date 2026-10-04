//! Orange desktop application, typed commands and background services.
//! The shared Dioxus UI lives in [`ui`] with optional native integrations.

pub mod about;
pub use orange_services::commands;
pub mod dialogs;
#[cfg(all(feature = "dbus", target_os = "linux"))]
pub use orange_services::mpris_host;
pub use orange_services::{files, library};
pub mod nav;
#[cfg(feature = "notify")]
pub use orange_services::notify;
pub mod playerbar;
pub use orange_services::{service, settings, state};

#[cfg(feature = "desktop")]
pub mod platform;
#[cfg(feature = "desktop")]
pub mod ui;

pub use orange_core::identity::APP_ID;
