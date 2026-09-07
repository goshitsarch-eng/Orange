//! Live MPRIS2 round-trip (feature `dbus`).
//!
//! Serves Orange MPRIS on a unique test bus name, then drives it as a
//! remote client would: reads Identity/PlaybackStatus/Metadata and sends
//! PlayPause. Skips gracefully without a session bus.

#![cfg(feature = "dbus")]

use std::collections::HashMap;
use std::sync::mpsc;
use std::time::Duration;

use orange_media::mpris::{MprisMetadata, PlaybackStatus, OBJECT_PATH};
use orange_media::mpris_client::{media_key_on, read_identity, MediaKey};
use orange_media::mpris_server::{serve, MprisCommand, MprisState, SharedState};

fn test_bus_name() -> String {
    format!("org.mpris.MediaPlayer2.OrangeTest{}", std::process::id())
}

async fn session_or_skip() -> Option<zbus::Connection> {
    match zbus::Connection::session().await {
        Ok(conn) => Some(conn),
        Err(e) => {
            eprintln!("SKIP: no session bus ({e})");
            None
        }
    }
}

#[tokio::test]
async fn mpris_round_trip() {
    let Some(client) = session_or_skip().await else {
        return;
    };
    let bus_name = test_bus_name();
    let state = SharedState::new(MprisState {
        status: PlaybackStatus::Playing,
        metadata: Some(MprisMetadata::from_song(
            "So What",
            "Miles Davis",
            "Kind of Blue",
            545_000_000_000,
            3,
            "",
        )),
        ..MprisState::default()
    });
    let (tx, rx) = mpsc::channel();
    let server = tokio::spawn(serve(bus_name.clone(), state, tx));

    // Wait for the name to appear (server startup is async).
    let deadline = std::time::Instant::now() + Duration::from_secs(10);
    let identity = loop {
        match read_identity(&client, &bus_name).await {
            Ok(identity) => break identity,
            Err(_) if std::time::Instant::now() < deadline => {
                tokio::time::sleep(Duration::from_millis(100)).await;
            }
            Err(e) => panic!("MPRIS server never appeared: {e}"),
        }
    };
    assert_eq!(identity, "Orange Music Player");

    let player = zbus::Proxy::new(
        &client,
        bus_name.as_str(),
        OBJECT_PATH,
        "org.mpris.MediaPlayer2.Player",
    )
    .await
    .unwrap();
    let status: String = player.get_property("PlaybackStatus").await.unwrap();
    assert_eq!(status, "Playing");
    let metadata: HashMap<String, zbus::zvariant::OwnedValue> =
        player.get_property("Metadata").await.unwrap();
    let title = String::try_from(metadata.get("xesam:title").unwrap().clone()).unwrap();
    assert_eq!(title, "So What");

    media_key_on(&client, &bus_name, MediaKey::PlayPause)
        .await
        .unwrap();
    let command = rx.recv_timeout(Duration::from_secs(5)).unwrap();
    assert_eq!(command, MprisCommand::PlayPause);

    server.abort();
}
