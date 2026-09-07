//! Discord Rich Presence over the local IPC socket (Unix, std only).
//!
//! Implements the Discord IPC framing (little-endian opcode + length +
//! JSON) and the handshake / `SET_ACTIVITY` payloads. Best-effort by
//! design: connecting fails cleanly when Discord is absent, and presence
//! never blocks playback. Mirrors the 2.1.5 Discord integration.

use std::path::PathBuf;

/// Rich Presence activity for the current track.
#[derive(Debug, Clone, PartialEq, Eq, Default)]
pub struct Activity {
    /// Track title (first line).
    pub details: String,
    /// Artist — album (second line).
    pub state: String,
    /// Unix seconds when playback started (elapsed timer).
    pub start_epoch_secs: Option<i64>,
}

/// Opcode for handshake frames.
pub const OP_HANDSHAKE: u32 = 0;
/// Opcode for command/event frames.
pub const OP_FRAME: u32 = 1;

/// Encode one IPC frame: opcode + length (both little-endian u32) + JSON.
pub fn encode_frame(opcode: u32, json: &str) -> Vec<u8> {
    let mut frame = Vec::with_capacity(8 + json.len());
    frame.extend_from_slice(&opcode.to_le_bytes());
    frame.extend_from_slice(&(json.len() as u32).to_le_bytes());
    frame.extend_from_slice(json.as_bytes());
    frame
}

/// Decode one IPC frame. Returns opcode + JSON on a complete buffer.
pub fn decode_frame(bytes: &[u8]) -> Option<(u32, String)> {
    if bytes.len() < 8 {
        return None;
    }
    let opcode = u32::from_le_bytes(bytes[0..4].try_into().ok()?);
    let length = u32::from_le_bytes(bytes[4..8].try_into().ok()?) as usize;
    let payload = bytes.get(8..8 + length)?;
    Some((opcode, String::from_utf8(payload.to_vec()).ok()?))
}

/// Minimal JSON string escaping for payload building.
pub fn escape_json(value: &str) -> String {
    let mut out = String::with_capacity(value.len() + 2);
    for c in value.chars() {
        match c {
            '"' => out.push_str("\\\""),
            '\\' => out.push_str("\\\\"),
            '\n' => out.push_str("\\n"),
            '\r' => out.push_str("\\r"),
            '\t' => out.push_str("\\t"),
            c if (c as u32) < 0x20 => out.push_str(&format!("\\u{:04x}", c as u32)),
            c => out.push(c),
        }
    }
    out
}

/// Handshake payload: protocol version plus our application ID.
pub fn handshake_payload(client_id: &str) -> String {
    format!("{{\"v\":1,\"client_id\":\"{}\"}}", escape_json(client_id))
}

/// `SET_ACTIVITY` payload for one track update.
pub fn set_activity_payload(pid: u32, nonce: &str, activity: Option<&Activity>) -> String {
    let activity_json = match activity {
        None => "null".to_string(),
        Some(activity) => {
            let timestamps = activity
                .start_epoch_secs
                .map(|start| format!(",\"timestamps\":{{\"start\":{start}}}"))
                .unwrap_or_default();
            format!(
                "{{\"details\":\"{}\",\"state\":\"{}\"{timestamps},\"assets\":{{\"large_image\":\"orange\",\"large_text\":\"Orange Music Player\"}}}}",
                escape_json(&activity.details),
                escape_json(&activity.state),
            )
        }
    };
    format!(
        "{{\"cmd\":\"SET_ACTIVITY\",\"args\":{{\"pid\":{pid},\"activity\":{activity_json}}},\"nonce\":\"{}\"}}",
        escape_json(nonce)
    )
}

/// Candidate IPC socket paths for a runtime dir (`discord-ipc-0` .. `9`).
pub fn ipc_socket_candidates(runtime_dir: Option<&str>) -> Vec<PathBuf> {
    let Some(dir) = runtime_dir.filter(|dir| !dir.is_empty()) else {
        return Vec::new();
    };
    (0..10)
        .map(|slot| PathBuf::from(format!("{dir}/discord-ipc-{slot}")))
        .collect()
}

/// Best-effort presence client. Connects on demand; every operation degrades
/// to `Ok`/`None` when Discord is absent.
#[cfg(unix)]
pub struct PresenceClient {
    stream: Option<std::os::unix::net::UnixStream>,
    client_id: String,
}

#[cfg(unix)]
impl PresenceClient {
    pub fn new(client_id: &str) -> Self {
        Self {
            stream: None,
            client_id: client_id.to_string(),
        }
    }

    /// Connect + handshake. `Ok(false)` when Discord is not running.
    pub fn connect(&mut self) -> std::io::Result<bool> {
        let runtime = std::env::var("XDG_RUNTIME_DIR").unwrap_or_default();
        let runtime = if runtime.is_empty() {
            None
        } else {
            Some(runtime)
        };
        self.connect_to(runtime.as_deref())
    }

    /// Same, with an explicit runtime dir (tests inject an empty dir).
    pub fn connect_to(&mut self, runtime_dir: Option<&str>) -> std::io::Result<bool> {
        use std::io::{Read, Write};
        use std::os::unix::net::UnixStream;

        for path in ipc_socket_candidates(runtime_dir) {
            let Ok(stream) = UnixStream::connect(&path) else {
                continue;
            };
            stream.set_read_timeout(Some(std::time::Duration::from_secs(2)))?;
            let mut stream = stream;
            let hello = encode_frame(OP_HANDSHAKE, &handshake_payload(&self.client_id));
            stream.write_all(&hello)?;
            // Best-effort READY read: a short header proves the handshake.
            let mut header = [0u8; 8];
            if stream.read_exact(&mut header).is_ok() {
                self.stream = Some(stream);
                return Ok(true);
            }
        }
        Ok(false)
    }

    /// Publish one activity update. Silently drops when disconnected.
    pub fn set_activity(&mut self, pid: u32, activity: Option<&Activity>) {
        use std::io::Write;
        let Some(stream) = self.stream.as_mut() else {
            return;
        };
        let frame = encode_frame(OP_FRAME, &set_activity_payload(pid, "orange", activity));
        if stream.write_all(&frame).is_err() {
            self.stream = None;
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn frames_round_trip() {
        let json = handshake_payload("1234");
        let frame = encode_frame(OP_HANDSHAKE, &json);
        assert_eq!(&frame[0..4], &0u32.to_le_bytes());
        let (opcode, back) = decode_frame(&frame).unwrap();
        assert_eq!((opcode, back.as_str()), (OP_HANDSHAKE, json.as_str()));
        assert!(decode_frame(&frame[..5]).is_none());
        assert!(decode_frame(b"\x01\x00\x00\x00\x05\x00\x00\x00abc").is_none());
    }

    #[test]
    fn payloads_escape_and_shape() {
        let activity = Activity {
            details: "Say \"hi\"".to_string(),
            state: "A\\B".to_string(),
            start_epoch_secs: Some(1700000000),
        };
        let payload = set_activity_payload(42, "n1", Some(&activity));
        assert!(payload.contains("\"cmd\":\"SET_ACTIVITY\""));
        assert!(payload.contains("Say \\\"hi\\\""));
        assert!(payload.contains("\"start\":1700000000"));
        assert!(payload.contains("\"pid\":42"));
        let cleared = set_activity_payload(42, "n2", None);
        assert!(cleared.contains("\"activity\":null"));
    }

    #[test]
    fn socket_candidates_shape() {
        let candidates = ipc_socket_candidates(Some("/run/user/1000"));
        assert_eq!(candidates.len(), 10);
        assert_eq!(candidates[0], PathBuf::from("/run/user/1000/discord-ipc-0"));
        assert!(ipc_socket_candidates(None).is_empty());
        assert!(ipc_socket_candidates(Some("")).is_empty());
    }

    #[cfg(unix)]
    #[test]
    fn absent_discord_connects_cleanly() {
        // An empty runtime dir cannot host Discord: must return Ok(false),
        // never Err, never block.
        let mut dir = std::env::temp_dir();
        dir.push(format!("orange-discord-{}", std::process::id()));
        std::fs::create_dir_all(&dir).unwrap();
        let mut client = PresenceClient::new("1234");
        assert!(!client.connect_to(Some(&dir.to_string_lossy())).unwrap());
        std::fs::remove_dir_all(&dir).ok();
    }
}
