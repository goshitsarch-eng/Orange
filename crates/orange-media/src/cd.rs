//! Audio CD: TOC reading, `cdda://` URLs, MusicBrainz DiscIDs.
//!
//! The table of contents comes from Linux `CDROMREADTOC*` ioctls (feature
//! `sys`, no libcdio needed); playback consumes `cdda://` URIs through the
//! GStreamer backend (`cdiocddasrc`). DiscIDs follow libdiscid bit-exactly
//! (feature `crypto`): SHA-1 over ASCII hex, custom base64 alphabet.

/// One audio track's start address in sectors (75 per second).
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub struct CdTocTrack {
    pub number: u8,
    pub start_lba: u32,
}

/// Table of contents of one audio disc.
#[derive(Debug, Clone, PartialEq, Eq)]
pub struct CdToc {
    pub first_track: u8,
    pub last_track: u8,
    pub tracks: Vec<CdTocTrack>,
    /// Lead-out address in sectors (end of the program area).
    pub leadout_lba: u32,
}

impl CdToc {
    /// Length of one track in sectors, if the TOC covers it.
    pub fn track_sectors(&self, number: u8) -> Option<u32> {
        let index = self
            .tracks
            .iter()
            .position(|track| track.number == number)?;
        let start = self.tracks[index].start_lba;
        let end = self
            .tracks
            .get(index + 1)
            .map(|next| next.start_lba)
            .unwrap_or(self.leadout_lba);
        end.checked_sub(start)
    }

    /// libdiscid TOC string: `first last leadout t1 t2 ...` (sectors).
    pub fn toc_string(&self) -> String {
        let mut parts = vec![
            self.first_track.to_string(),
            self.last_track.to_string(),
            self.leadout_lba.to_string(),
        ];
        parts.extend(self.tracks.iter().map(|track| track.start_lba.to_string()));
        parts.join(" ")
    }

    /// 100 offsets for the DiscID preimage: index 0 is the lead-out,
    /// indices 1-99 the track starts (missing entries are zero).
    pub fn offsets_for_discid(&self) -> [u32; 100] {
        let mut offsets = [0u32; 100];
        offsets[0] = self.leadout_lba;
        for track in &self.tracks {
            if track.number >= 1 && track.number <= 99 {
                offsets[track.number as usize] = track.start_lba;
            }
        }
        offsets
    }

    /// ASCII preimage hashed for the DiscID: `%02X` first/last plus 100
    /// `%08X` offsets, exactly like libdiscid's `create_disc_id`.
    pub fn disc_id_preimage(&self) -> String {
        let mut preimage = format!("{:02X}{:02X}", self.first_track, self.last_track);
        for offset in self.offsets_for_discid() {
            preimage.push_str(&format!("{offset:08X}"));
        }
        preimage
    }

    /// MusicBrainz DiscID (feature `crypto`): SHA-1 of the preimage in the
    /// libdiscid base64 alphabet (`A-Za-z0-9._`, pad `-`).
    #[cfg(feature = "crypto")]
    pub fn disc_id(&self) -> String {
        use sha1::{Digest, Sha1};
        let mut hasher = Sha1::new();
        hasher.update(self.disc_id_preimage().as_bytes());
        custom_base64(&hasher.finalize())
    }
}

/// Custom base64 for DiscIDs: standard alphabet with `+/=` mapped to `._-`.
#[cfg(feature = "crypto")]
fn custom_base64(digest: &[u8]) -> String {
    const ALPHABET: &[u8] = b"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._";
    let mut out = String::with_capacity(28);
    for chunk in digest.chunks(3) {
        let mut bits: u32 = 0;
        for (index, byte) in chunk.iter().enumerate() {
            bits |= (*byte as u32) << (16 - 8 * index);
        }
        let chars = match chunk.len() {
            3 => 4,
            2 => 3,
            _ => 2,
        };
        for position in 0..chars {
            let shift = 18 - 6 * position;
            out.push(ALPHABET[((bits >> shift) & 0x3F) as usize] as char);
        }
        for _ in chars..4 {
            out.push('-');
        }
    }
    out
}

/// Parse our canonical `cdda://<device>/<track>` URL back into parts.
/// Mirrors [`crate::devices::CdTrack::url`].
pub fn parse_cdda_url(url: &str) -> Option<(String, u32)> {
    let rest = url.strip_prefix("cdda://")?;
    let slash = rest.rfind('/')?;
    let (device, track) = rest.split_at(slash);
    if device.is_empty() {
        return None;
    }
    Some((device.to_string(), track[1..].parse().ok()?))
}

/// MusicBrainz webservice URL for a DiscID lookup.
pub fn discid_lookup_url(disc_id: &str) -> String {
    format!("https://musicbrainz.org/ws/2/discid/{disc_id}?fmt=json")
}

/// Read the TOC of an optical drive via Linux CDROM ioctls.
/// Returns an I/O error when the drive is missing, empty, or not a CD.
#[cfg(all(feature = "sys", target_os = "linux"))]
pub fn read_toc_linux(device: &str) -> std::io::Result<CdToc> {
    use std::os::unix::io::AsRawFd;

    const CDROMREADTOCHDR: libc::c_ulong = 0x5305;
    const CDROMREADTOCENTRY: libc::c_ulong = 0x5306;
    const CDTE_FORMAT_LBA: u8 = 0x02;
    const CDROM_LEADOUT: u8 = 0xAA;

    #[repr(C)]
    struct Tochdr {
        first: u8,
        last: u8,
    }

    #[repr(C)]
    #[derive(Default)]
    struct Tocentry {
        track: u8,
        adr_ctrl: u8,
        format: u8,
        addr_lba: i32,
        datamode: u8,
    }

    // SAFETY: plain-old-data structs passed to a read-only kernel ioctl.
    unsafe fn ioctl_entry(fd: i32, track: u8) -> std::io::Result<u32> {
        let mut entry = Tocentry {
            track,
            format: CDTE_FORMAT_LBA,
            ..Tocentry::default()
        };
        let result = libc::ioctl(fd, CDROMREADTOCENTRY, &mut entry);
        if result < 0 {
            return Err(std::io::Error::last_os_error());
        }
        Ok(entry.addr_lba as u32)
    }

    let file = std::fs::File::open(device)?;
    let fd = file.as_raw_fd();
    let mut header = Tochdr { first: 0, last: 0 };
    // SAFETY: header is a POD out-parameter for a read-only ioctl.
    let result = unsafe { libc::ioctl(fd, CDROMREADTOCHDR, &mut header) };
    if result < 0 {
        return Err(std::io::Error::last_os_error());
    }
    if header.last < header.first || header.first == 0 {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "not an audio CD table of contents",
        ));
    }
    let mut tracks = Vec::new();
    for number in header.first..=header.last {
        // SAFETY: same contract as above.
        let start_lba = unsafe { ioctl_entry(fd, number)? };
        tracks.push(CdTocTrack { number, start_lba });
    }
    // SAFETY: same contract as above.
    let leadout_lba = unsafe { ioctl_entry(fd, CDROM_LEADOUT)? };
    Ok(CdToc {
        first_track: header.first,
        last_track: header.last,
        tracks,
        leadout_lba,
    })
}

#[cfg(test)]
mod tests {
    use super::*;

    pub(crate) fn fixture_toc() -> CdToc {
        CdToc {
            first_track: 1,
            last_track: 3,
            tracks: vec![
                CdTocTrack {
                    number: 1,
                    start_lba: 150,
                },
                CdTocTrack {
                    number: 2,
                    start_lba: 5000,
                },
                CdTocTrack {
                    number: 3,
                    start_lba: 10000,
                },
            ],
            leadout_lba: 15000,
        }
    }

    #[test]
    fn toc_string_and_lengths() {
        let toc = fixture_toc();
        assert_eq!(toc.toc_string(), "1 3 15000 150 5000 10000");
        assert_eq!(toc.track_sectors(1), Some(4850));
        assert_eq!(toc.track_sectors(2), Some(5000));
        assert_eq!(toc.track_sectors(3), Some(5000));
        assert_eq!(toc.track_sectors(4), None);
    }

    #[test]
    fn discid_preimage_shape() {
        let toc = fixture_toc();
        let preimage = toc.disc_id_preimage();
        assert!(preimage.starts_with("0103"));
        // 2 + 2 hex chars plus 100 x 8 hex chars.
        assert_eq!(preimage.len(), 4 + 800);
        assert!(preimage
            .chars()
            .all(|c| c.is_ascii_hexdigit() && !c.is_ascii_lowercase()));
    }

    #[cfg(feature = "crypto")]
    #[test]
    fn disc_id_matches_libdiscid_oracle() {
        // Independent oracle: CPython hashlib + translated base64 of the
        // libdiscid preimage for this TOC.
        assert_eq!(fixture_toc().disc_id(), "uwKOjclBU9Mm8RGe_dqY85OQvlE-");
        assert_eq!(fixture_toc().disc_id().len(), 28);
    }

    #[test]
    fn cdda_urls_round_trip() {
        assert_eq!(
            parse_cdda_url("cdda:///dev/sr0/3"),
            Some(("/dev/sr0".to_string(), 3))
        );
        assert!(parse_cdda_url("file:///music/a.flac").is_none());
        assert!(parse_cdda_url("cdda:///dev/sr0/x").is_none());
        assert!(discid_lookup_url("abc").ends_with("/discid/abc?fmt=json"));
    }

    #[cfg(all(feature = "sys", target_os = "linux"))]
    #[test]
    fn missing_drive_is_an_io_error() {
        // No fixture drive here: proves graceful failure, never a panic.
        let result = read_toc_linux("/dev/orange-test-nonexistent-sr9");
        assert!(result.is_err());
    }
}
