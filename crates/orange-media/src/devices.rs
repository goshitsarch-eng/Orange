//! Devices, Audio CD, and transcoding.
//! Mirrors `device` (USB mass-storage, MTP, iPod Classic), the Audio CD
//! path, `organize`, and `transcoder`.

use orange_core::codecs::{TranscodeTarget, TRANSCODE_TARGETS};

/// Device families Orange syncs music to, mirroring 2.1.5 support.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum DeviceFamily {
    /// Mass-storage USB players.
    UsbMassStorage,
    /// MTP devices (libmtp path).
    Mtp,
    /// iPod Nano/Classic (libgpod path).
    IPod,
}

impl DeviceFamily {
    pub fn name(self) -> &'static str {
        match self {
            Self::UsbMassStorage => "USB",
            Self::Mtp => "MTP",
            Self::IPod => "iPod",
        }
    }
}

/// A connected device. `id` is the stable per-device identifier used for
/// the per-device database (`device-schema.sql`); never a mount path.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Device {
    pub id: String,
    pub family: DeviceFamily,
    pub label: String,
    pub capacity_bytes: Option<u64>,
}

/// One sync job: copy (optionally transcoded) tracks to a device.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct DeviceSyncJob {
    pub device_id: String,
    pub urls: Vec<String>,
    pub transcode: Option<TranscodeTarget>,
    pub overwrite: bool,
}

impl DeviceSyncJob {
    pub fn is_valid(&self) -> bool {
        !self.device_id.is_empty() && !self.urls.is_empty()
    }
}

/// Audio CD track reference, mirroring the libcdio playback path.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CdTrack {
    pub device: String,
    pub track_number: u32,
    pub length_secs: i64,
}

impl CdTrack {
    pub fn url(&self) -> String {
        format!(
            "cdda://{}/{}",
            self.device.trim_end_matches('/'),
            self.track_number
        )
    }
}

/// A transcode job: input URL to output with a target preset.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct TranscodeJob {
    pub input_url: String,
    pub target: TranscodeTarget,
}

impl TranscodeJob {
    /// Output filename for `input_url` under the target preset.
    pub fn output_name(&self, title: &str) -> Option<String> {
        if self.input_url.is_empty() || title.trim().is_empty() {
            return None;
        }
        Some(format!(
            "{}.{}",
            sanitize_filename(title),
            self.target.extension
        ))
    }
}

fn sanitize_filename(title: &str) -> String {
    title
        .chars()
        .map(|c| {
            if matches!(c, '/' | '\\' | ':' | '*' | '?' | '"' | '<' | '>' | '|') {
                '_'
            } else {
                c
            }
        })
        .collect::<String>()
        .trim()
        .to_string()
}

/// Look up a transcode target by display name (case-insensitive).
pub fn transcode_target(name: &str) -> Option<TranscodeTarget> {
    TRANSCODE_TARGETS
        .iter()
        .find(|t| t.name.eq_ignore_ascii_case(name.trim()))
        .copied()
}

// ---------------------------------------------------------------------------
// Mounts and the sync executor (std only; UDisks2 detection in `udisks`).
// ---------------------------------------------------------------------------

/// One mounted filesystem from `/proc/mounts`.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct Mount {
    pub device: String,
    pub path: String,
    pub fstype: String,
}

/// Parse `/proc/mounts` content (`device path fstype options ...`).
pub fn parse_proc_mounts(text: &str) -> Vec<Mount> {
    text.lines()
        .filter_map(|line| {
            let line = line.trim();
            if line.is_empty() || line.starts_with('#') {
                return None;
            }
            let mut parts = line.split_whitespace();
            Some(Mount {
                device: parts.next()?.to_string(),
                path: parts.next()?.to_string(),
                fstype: parts.next()?.to_string(),
            })
        })
        .collect()
}

/// Filesystems typical of removable media players and sticks.
const REMOVABLE_FSTYPES: &[&str] = &["vfat", "exfat", "ntfs-3g", "ntfs3", "fuseblk"];

/// Mounts that look like removable devices: known removable filesystems, or
/// anything under the desktop media roots (covers ext4-formatted players).
pub fn removable_mounts(mounts: &[Mount]) -> Vec<&Mount> {
    mounts
        .iter()
        .filter(|mount| {
            REMOVABLE_FSTYPES.contains(&mount.fstype.as_str())
                || mount.path.starts_with("/run/media/")
                || mount.path.starts_with("/media/")
        })
        .collect()
}

/// One track queued for device sync.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SyncTrack {
    /// `file://` source URL.
    pub source_url: String,
    pub artist: String,
    pub album: String,
    pub title: String,
    pub track_no: u32,
}

/// Organize destination: `Artist/Album/NN - Title.<ext>`, sanitized.
pub fn organize_relative_path(track: &SyncTrack, extension: &str) -> std::path::PathBuf {
    let number = format!("{:02}", track.track_no.min(99));
    let file = format!("{number} - {}.{extension}", sanitize_filename(&track.title));
    std::path::PathBuf::from(sanitize_filename(&track.artist))
        .join(sanitize_filename(&track.album))
        .join(file)
}

/// Sync progress callback payload.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SyncProgress {
    pub done: usize,
    pub total: usize,
    pub current: String,
}

/// Sync report.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct SyncReport {
    pub copied: usize,
    pub transcoded: usize,
    pub skipped: usize,
}

/// One-file converter used for transcoding syncs.
pub type TranscodeOne = dyn Fn(&std::path::Path, &std::path::Path) -> Result<(), String>;

/// Sync failure.
#[derive(Debug)]
pub struct SyncError(pub String);

impl std::fmt::Display for SyncError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "sync error: {}", self.0)
    }
}

impl std::error::Error for SyncError {}

/// Copy (and optionally transcode) tracks onto a mounted device.
///
/// `file_url_to_path` maps `file://` URLs to local paths. `transcode_one`
/// converts one source file to the destination (e.g. via the GStreamer
/// backend); when `None`, sources copy verbatim and the destination keeps
/// the source extension.
#[allow(clippy::too_many_arguments)]
pub fn execute_sync(
    dest_root: &std::path::Path,
    tracks: &[SyncTrack],
    overwrite: bool,
    transcode_target: Option<TranscodeTarget>,
    on_progress: &mut dyn FnMut(SyncProgress),
    transcode_one: Option<&TranscodeOne>,
) -> Result<SyncReport, SyncError> {
    let mut report = SyncReport {
        copied: 0,
        transcoded: 0,
        skipped: 0,
    };
    for (index, track) in tracks.iter().enumerate() {
        on_progress(SyncProgress {
            done: index,
            total: tracks.len(),
            current: track.title.clone(),
        });
        let source = file_url_to_path(&track.source_url)
            .ok_or_else(|| SyncError(format!("unsupported source URL: {}", track.source_url)))?;
        let extension = match &transcode_target {
            Some(target) => target.extension.to_string(),
            None => source
                .extension()
                .and_then(|ext| ext.to_str())
                .unwrap_or("bin")
                .to_string(),
        };
        let dest = dest_root.join(organize_relative_path(track, &extension));
        if dest.exists() && !overwrite {
            report.skipped += 1;
            continue;
        }
        if let Some(parent) = dest.parent() {
            std::fs::create_dir_all(parent)
                .map_err(|e| SyncError(format!("cannot create {}: {e}", parent.display())))?;
        }
        if transcode_target.is_some() {
            let convert = transcode_one.ok_or_else(|| {
                SyncError("transcode requested but no converter given".to_string())
            })?;
            convert(&source, &dest)
                .map_err(|e| SyncError(format!("transcode failed for {}: {e}", track.title)))?;
            report.transcoded += 1;
        } else {
            std::fs::copy(&source, &dest)
                .map_err(|e| SyncError(format!("copy failed for {}: {e}", track.title)))?;
            report.copied += 1;
        }
    }
    on_progress(SyncProgress {
        done: tracks.len(),
        total: tracks.len(),
        current: String::new(),
    });
    Ok(report)
}

/// Map a `file://` URL to a local path. Anything else is unsupported.
pub fn file_url_to_path(url: &str) -> Option<std::path::PathBuf> {
    url.strip_prefix("file://")
        .filter(|rest| !rest.is_empty())
        .map(std::path::PathBuf::from)
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn device_families() {
        assert_eq!(DeviceFamily::Mtp.name(), "MTP");
        assert_eq!(DeviceFamily::IPod.name(), "iPod");
        let job = DeviceSyncJob {
            device_id: "usb-1".into(),
            urls: vec!["a".into()],
            transcode: None,
            overwrite: false,
        };
        assert!(job.is_valid());
        assert!(!DeviceSyncJob {
            device_id: String::new(),
            urls: vec![],
            transcode: None,
            overwrite: false
        }
        .is_valid());
    }

    #[test]
    fn cd_track_url() {
        let track = CdTrack {
            device: "/dev/sr0".into(),
            track_number: 3,
            length_secs: 185,
        };
        assert_eq!(track.url(), "cdda:///dev/sr0/3");
    }

    #[test]
    fn transcode_output_names() {
        let job = TranscodeJob {
            input_url: "file:///a.flac".into(),
            target: transcode_target("mp3").unwrap(),
        };
        assert_eq!(
            job.output_name("Blue/Green: Live?").unwrap(),
            "Blue_Green_ Live_.mp3"
        );
        assert!(TranscodeJob {
            input_url: String::new(),
            target: job.target
        }
        .output_name("x")
        .is_none());
        assert!(transcode_target("bogus").is_none());
    }

    const PROC_MOUNTS: &str = "\
/dev/sda2 / ext4 rw,relatime 0 0\n\
/dev/sdb1 /run/media/gosh/WALKMAN vfat rw,nosuid 0 0\n\
/dev/sr0 /run/media/gosh/AudioCD udf ro,nosuid 0 0\n\
tmpfs /run/user/1000 tmpfs rw,nosuid 0 0\n";

    #[test]
    fn mounts_parse_and_filter() {
        let mounts = parse_proc_mounts(PROC_MOUNTS);
        assert_eq!(mounts.len(), 4);
        let removable = removable_mounts(&mounts);
        let paths: Vec<&str> = removable.iter().map(|mount| mount.path.as_str()).collect();
        assert!(paths.contains(&"/run/media/gosh/WALKMAN"));
        assert!(!paths.contains(&"/"));
        assert!(!paths.contains(&"/run/user/1000"));
    }

    #[test]
    fn organize_paths_sanitize() {
        let track = SyncTrack {
            source_url: "file:///music/a.flac".to_string(),
            artist: "Miles/Davis".to_string(),
            album: "Kind of Blue".to_string(),
            title: "So What?".to_string(),
            track_no: 1,
        };
        assert_eq!(
            organize_relative_path(&track, "mp3"),
            std::path::PathBuf::from("Miles_Davis/Kind of Blue/01 - So What_.mp3")
        );
    }

    fn temp_sync_dir(name: &str) -> std::path::PathBuf {
        let mut dir = std::env::temp_dir();
        dir.push(format!("orange-sync-{name}-{}", std::process::id()));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    fn sync_track(source: &std::path::Path) -> SyncTrack {
        SyncTrack {
            source_url: format!("file://{}", source.display()),
            artist: "Miles Davis".to_string(),
            album: "Kind of Blue".to_string(),
            title: "So What".to_string(),
            track_no: 1,
        }
    }

    #[test]
    fn sync_copies_and_skips() {
        let work = temp_sync_dir("copy");
        let source = work.join("a.flac");
        std::fs::write(&source, b"fake-flac").unwrap();
        let dest = work.join("device");
        let tracks = vec![sync_track(&source)];
        let mut progress = Vec::new();
        let report = execute_sync(
            &dest,
            &tracks,
            false,
            None,
            &mut |update: SyncProgress| progress.push(update),
            None,
        )
        .unwrap();
        assert_eq!((report.copied, report.skipped), (1, 0));
        assert!(dest
            .join("Miles Davis/Kind of Blue/01 - So What.flac")
            .exists());
        assert_eq!(progress.last().unwrap().done, 1);
        // Second run without overwrite skips.
        let report =
            execute_sync(&dest, &tracks, false, None, &mut |_: SyncProgress| {}, None).unwrap();
        assert_eq!((report.copied, report.skipped), (0, 1));
        std::fs::remove_dir_all(&work).ok();
    }

    #[test]
    fn sync_transcodes_through_callback() {
        let work = temp_sync_dir("transcode");
        let source = work.join("a.flac");
        std::fs::write(&source, b"fake-flac").unwrap();
        let dest = work.join("device");
        let tracks = vec![sync_track(&source)];
        let mut on_progress = |_: SyncProgress| {};
        let convert = |from: &std::path::Path, to: &std::path::Path| {
            let bytes = std::fs::read(from).map_err(|e| e.to_string())?;
            std::fs::write(to, [b"mp3", bytes.as_slice()].concat()).map_err(|e| e.to_string())
        };
        let report = execute_sync(
            &dest,
            &tracks,
            true,
            transcode_target("mp3"),
            &mut on_progress,
            Some(&convert),
        )
        .unwrap();
        assert_eq!((report.transcoded, report.copied), (1, 0));
        assert!(dest
            .join("Miles Davis/Kind of Blue/01 - So What.mp3")
            .exists());
        // Transcode requested without a converter is an error, not a copy.
        assert!(execute_sync(
            &dest,
            &tracks,
            true,
            transcode_target("mp3"),
            &mut on_progress,
            None
        )
        .is_err());
        std::fs::remove_dir_all(&work).ok();
    }

    #[test]
    fn file_url_mapping() {
        assert_eq!(
            file_url_to_path("file:///music/a.flac"),
            Some(std::path::PathBuf::from("/music/a.flac"))
        );
        assert!(file_url_to_path("https://example.com/a.mp3").is_none());
        assert!(file_url_to_path("file://").is_none());
    }
}
