//! MPRIS2 client (feature `dbus`): one-shot remote commands for the CLI
//! media-key flags (`orange --play-pause`, `--stop`, ...) and desktop
//! actions. Talks to the running instance at [`crate::mpris::BUS_NAME`].

use zbus::Proxy;

use crate::mpris;

/// Remote command the CLI can send.
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum MediaKey {
    Play,
    Pause,
    PlayPause,
    Stop,
    Next,
    Previous,
}

impl MediaKey {
    fn method(self) -> &'static str {
        match self {
            Self::Play => "Play",
            Self::Pause => "Pause",
            Self::PlayPause => "PlayPause",
            Self::Stop => "Stop",
            Self::Next => "Next",
            Self::Previous => "Previous",
        }
    }

    /// Parse a CLI flag tail (`play-pause`, `stop`, ...).
    pub fn parse(flag: &str) -> Option<Self> {
        match flag.trim_start_matches("--").to_ascii_lowercase().as_str() {
            "play" => Some(Self::Play),
            "pause" => Some(Self::Pause),
            "play-pause" => Some(Self::PlayPause),
            "stop" => Some(Self::Stop),
            "next" => Some(Self::Next),
            "previous" | "prev" => Some(Self::Previous),
            _ => None,
        }
    }
}

/// Send one media key to the running Orange instance.
pub async fn send_media_key(command: MediaKey) -> Result<(), String> {
    let conn = Connection::session().await.map_err(|e| e.to_string())?;
    media_key_on(&conn, mpris::BUS_NAME, command).await
}

use zbus::Connection;

/// Same, but over an existing connection (tests inject their own server).
pub async fn media_key_on(
    conn: &Connection,
    bus_name: &str,
    command: MediaKey,
) -> Result<(), String> {
    let proxy = Proxy::new(
        conn,
        bus_name,
        mpris::OBJECT_PATH,
        "org.mpris.MediaPlayer2.Player",
    )
    .await
    .map_err(|e| e.to_string())?;
    proxy
        .call_method(command.method(), &())
        .await
        .map_err(|e| e.to_string())?;
    Ok(())
}

/// Ask the running instance to stop after the current track (Orange
/// extension interface, driven by the desktop `StopAfterCurrent` action).
pub async fn send_stop_after_current(conn: &Connection, bus_name: &str) -> Result<(), String> {
    let proxy = Proxy::new(
        conn,
        bus_name,
        mpris::OBJECT_PATH,
        "org.goshapps.Orange.Player",
    )
    .await
    .map_err(|e| e.to_string())?;
    proxy
        .call_method("StopAfterCurrent", &())
        .await
        .map_err(|e| e.to_string())?;
    Ok(())
}

/// Read the `Identity` property (used to prove the server answers).
pub async fn read_identity(conn: &Connection, bus_name: &str) -> Result<String, String> {
    let proxy = Proxy::new(conn, bus_name, mpris::OBJECT_PATH, "org.mpris.MediaPlayer2")
        .await
        .map_err(|e| e.to_string())?;
    proxy
        .get_property::<String>("Identity")
        .await
        .map_err(|e| e.to_string())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn flag_parsing() {
        assert_eq!(MediaKey::parse("--play-pause"), Some(MediaKey::PlayPause));
        assert_eq!(MediaKey::parse("stop"), Some(MediaKey::Stop));
        assert_eq!(MediaKey::parse("--previous"), Some(MediaKey::Previous));
        assert_eq!(MediaKey::parse("--bogus"), None);
    }
}
