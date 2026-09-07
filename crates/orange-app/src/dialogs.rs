//! Cosmic dialog models: tag editor, transcoder, device sync, scrobbler auth.
//! Each dialog is pure state + validation; the `ui` feature renders it as a
//! `cosmic::widget::dialog`. Scrobbler auth never exposes secrets.

use orange_core::codecs::TranscodeTarget;
use orange_media::devices::transcode_target;
use orange_media::online::{ScrobbleService, Secret};

/// Tag editor draft for one file. `apply` validates before the caller writes.
#[derive(Debug, Clone, Default)]
pub struct TagEditor {
    pub url: String,
    pub title: String,
    pub artist: String,
    pub album: String,
    pub genre: String,
    pub year: String,
    pub track: String,
}

impl TagEditor {
    pub fn is_valid(&self) -> bool {
        if self.url.trim().is_empty() {
            return false;
        }
        if !self.year.trim().is_empty() && self.year.trim().parse::<i32>().is_err() {
            return false;
        }
        if !self.track.trim().is_empty()
            && self
                .track
                .trim()
                .parse::<u32>()
                .ok()
                .filter(|&n| n > 0)
                .is_none()
        {
            return false;
        }
        true
    }

    /// Numbered validation errors for the dialog to display.
    pub fn errors(&self) -> Vec<&'static str> {
        let mut errors = Vec::new();
        if self.url.trim().is_empty() {
            errors.push("No file selected.");
        }
        if !self.year.trim().is_empty() && self.year.trim().parse::<i32>().is_err() {
            errors.push("Year must be a number.");
        }
        if !self.track.trim().is_empty()
            && self
                .track
                .trim()
                .parse::<u32>()
                .ok()
                .filter(|&n| n > 0)
                .is_none()
        {
            errors.push("Track must be a positive number.");
        }
        errors
    }
}

/// Transcoder dialog: target preset plus output directory.
#[derive(Debug, Clone)]
pub struct TranscoderDialog {
    pub target_name: String,
    pub output_dir: String,
}

impl TranscoderDialog {
    pub fn target(&self) -> Option<TranscodeTarget> {
        transcode_target(&self.target_name)
    }

    pub fn is_valid(&self) -> bool {
        self.target().is_some() && !self.output_dir.trim().is_empty()
    }
}

/// Device sync dialog: target device plus optional transcode preset.
#[derive(Debug, Clone)]
pub struct DeviceSyncDialog {
    pub device_id: String,
    pub transcode_name: Option<String>,
    pub overwrite: bool,
}

impl DeviceSyncDialog {
    pub fn transcode(&self) -> Option<TranscodeTarget> {
        self.transcode_name.as_deref().and_then(transcode_target)
    }

    pub fn is_valid(&self) -> bool {
        if self.device_id.trim().is_empty() {
            return false;
        }
        match self.transcode_name.as_deref() {
            None => true,
            Some(name) => transcode_target(name).is_some(),
        }
    }
}

/// Scrobbler auth dialog state machine. The secret is write-only: once
/// submitted it leaves as an opaque [`Secret`] and never renders back.
#[derive(Debug, Clone, PartialEq, Eq)]
pub enum ScrobblerAuth {
    SignedOut,
    AwaitingCredentials {
        service: ScrobbleService,
        username: String,
    },
    SignedIn {
        service: ScrobbleService,
        username: String,
    },
}

impl ScrobblerAuth {
    pub fn sign_in(
        service: ScrobbleService,
        username: &str,
        password: &str,
    ) -> (Self, Option<Secret>) {
        if username.trim().is_empty() || password.is_empty() {
            return (
                Self::AwaitingCredentials {
                    service,
                    username: username.to_string(),
                },
                None,
            );
        }
        // The password is sealed immediately; only the opaque handle survives.
        let secret = Secret::new(password.to_string());
        (
            Self::SignedIn {
                service,
                username: username.trim().to_string(),
            },
            Some(secret),
        )
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn tag_editor_validation() {
        let mut editor = TagEditor::default();
        assert!(!editor.is_valid());
        editor.url = "file:///m/a.flac".into();
        editor.year = "nineteen".into();
        assert!(!editor.is_valid());
        assert!(editor.errors().contains(&"Year must be a number."));
        editor.year = "1959".into();
        editor.track = "2".into();
        assert!(editor.is_valid());
    }

    #[test]
    fn transcoder_needs_target_and_dir() {
        let dialog = TranscoderDialog {
            target_name: "Opus".into(),
            output_dir: "/tmp/out".into(),
        };
        assert!(dialog.is_valid());
        assert!(!TranscoderDialog {
            target_name: "Nope".into(),
            output_dir: "/tmp".into()
        }
        .is_valid());
        assert!(!TranscoderDialog {
            target_name: "FLAC".into(),
            output_dir: "  ".into()
        }
        .is_valid());
    }

    #[test]
    fn device_sync_defaults_sane() {
        let dialog = DeviceSyncDialog {
            device_id: "ipod-1".into(),
            transcode_name: None,
            overwrite: false,
        };
        assert!(dialog.is_valid());
        assert!(!DeviceSyncDialog {
            device_id: String::new(),
            transcode_name: None,
            overwrite: false
        }
        .is_valid());
    }

    #[test]
    fn scrobbler_auth_seals_password() {
        let (state, secret) = ScrobblerAuth::sign_in(ScrobbleService::LastFm, "gosh", "s3cret");
        assert!(matches!(state, ScrobblerAuth::SignedIn { .. }));
        let secret = secret.expect("valid credentials produce a secret");
        assert_eq!(format!("{secret:?}"), "Secret([redacted])");
        let (retry, none) = ScrobblerAuth::sign_in(ScrobbleService::LastFm, "gosh", "");
        assert!(none.is_none());
        assert!(matches!(retry, ScrobblerAuth::AwaitingCredentials { .. }));
    }
}
