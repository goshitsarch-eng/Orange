//! `orange` binary: headless summary by default, COSMIC window with `--ui`,
//! MPRIS daemon with `--serve`, one-shot remotes for the desktop actions.
//!
//! - `orange --version` prints `orange 3.0.0`.
//! - `orange` (no flags) launches headless: opens the existing Orange
//!   collection read-only and prints a library summary. No Qt is involved
//!   at any point, and Strawberry data is never touched.
//! - `orange --ui` opens the native libcosmic window (requires the `ui`
//!   feature; always enabled in the Flatpak).
//! - `orange --serve [FILES...]` publishes MPRIS and plays the queue,
//!   driving real audio when the `gst` backend can start (requires `dbus`).
//! - `orange --play-pause|--stop|--stop-after-current|--previous|--next`
//!   controls the running instance over MPRIS (requires `dbus`).

use orange_core::identity;
use orange_core::version::{self, MAKER};

/// Parsed command line. Pure and unit-tested below.
#[derive(Debug, PartialEq, Eq)]
enum Command {
    Version,
    Help,
    Ui,
    Serve { uris: Vec<String> },
    MediaKey(String),
    Headless,
}

/// Split arguments into a [`Command`]. First match wins, mirroring the
/// desktop file (`Exec=orange --play-pause`, ...).
fn parse_args(args: &[String]) -> Command {
    if args.iter().any(|a| a == "--version" || a == "-V") {
        return Command::Version;
    }
    if args.iter().any(|a| a == "--help" || a == "-h") {
        return Command::Help;
    }
    if args.iter().any(|a| a == "--ui") {
        return Command::Ui;
    }
    if let Some(position) = args.iter().position(|a| a == "--serve") {
        let uris = args[position + 1..]
            .iter()
            .filter(|arg| !arg.starts_with("--"))
            .map(|arg| path_to_uri(arg))
            .collect();
        return Command::Serve { uris };
    }
    for flag in [
        "--play-pause",
        "--play",
        "--pause",
        "--stop",
        "--stop-after-current",
        "--previous",
        "--next",
    ] {
        if args.iter().any(|a| a == flag) {
            return Command::MediaKey(flag.to_string());
        }
    }
    Command::Headless
}

/// CLI path to `file://` URI. Remote URLs pass through untouched.
fn path_to_uri(arg: &str) -> String {
    if arg.contains("://") {
        return arg.to_string();
    }
    let path = std::path::Path::new(arg);
    let absolute = if path.is_absolute() {
        path.to_path_buf()
    } else {
        std::env::current_dir()
            .unwrap_or_else(|_| std::path::PathBuf::from("."))
            .join(path)
    };
    format!("file://{}", absolute.display())
}

fn main() {
    let args: Vec<String> = std::env::args().skip(1).collect();
    match parse_args(&args) {
        Command::Version => println!("{}", version::version_line()),
        Command::Help => print_help(),
        Command::Ui => run_ui(),
        Command::Serve { uris } => {
            std::process::exit(run_serve(uris));
        }
        Command::MediaKey(flag) => {
            std::process::exit(run_media_key(&flag));
        }
        Command::Headless => run_headless(),
    }
}

fn print_help() {
    println!(
        "Orange Music Player {} — Made by {}\n\n\
     Usage: orange [OPTION] [FILES...]\n\n\
     Options:\n  \
     --version   print version and exit\n  \
     --help      print this help and exit\n  \
     --ui        open the native COSMIC window (needs the `ui` feature)\n  \
     --serve     publish MPRIS and play FILES/the queue (needs `dbus`)\n  \
     --play-pause/--play/--pause/--stop/--previous/--next\n                 control the running instance over MPRIS\n  \
     --stop-after-current\n                 stop when the current track ends\n\n\
     With no option, orange launches headless: it opens your existing\n\
     collection read-only and prints a summary. Your Strawberry data is\n\
     left untouched.",
        version::VERSION,
        MAKER
    );
}

fn run_ui() {
    #[cfg(feature = "ui")]
    {
        if let Err(e) = orange_app::ui::run() {
            eprintln!("orange: failed to start COSMIC shell: {e}");
            std::process::exit(1);
        }
    }
    #[cfg(not(feature = "ui"))]
    {
        eprintln!("orange: this build has no COSMIC UI; rebuild with --features orange-app/ui.");
        std::process::exit(2);
    }
}

fn run_serve(uris: Vec<String>) -> i32 {
    #[cfg(feature = "dbus")]
    {
        return orange_app::mpris_host::serve_forever(uris);
    }
    #[cfg(not(feature = "dbus"))]
    {
        let _ = uris;
        eprintln!(
            "orange: this build has no MPRIS daemon; rebuild with --features orange-app/dbus."
        );
        2
    }
}

fn run_media_key(flag: &str) -> i32 {
    #[cfg(feature = "dbus")]
    {
        return dispatch_media_key(flag);
    }
    #[cfg(not(feature = "dbus"))]
    {
        let _ = flag;
        eprintln!(
            "orange: this build has no MPRIS client; rebuild with --features orange-app/dbus."
        );
        2
    }
}

/// Tokio-backed MPRIS dispatch (feature `dbus` only).
#[cfg(feature = "dbus")]
fn dispatch_media_key(flag: &str) -> i32 {
    use orange_media::mpris::BUS_NAME;
    use orange_media::mpris_client::{media_key_on, send_stop_after_current, MediaKey};

    let runtime = match tokio::runtime::Builder::new_current_thread()
        .enable_all()
        .build()
    {
        Ok(runtime) => runtime,
        Err(e) => {
            eprintln!("orange: async runtime failed: {e}");
            return 1;
        }
    };
    let result = runtime.block_on(async {
        let conn = zbus::Connection::session()
            .await
            .map_err(|e| format!("no session bus: {e}"))?;
        if flag == "--stop-after-current" {
            return send_stop_after_current(&conn, BUS_NAME).await;
        }
        let key = MediaKey::parse(flag).ok_or_else(|| format!("unknown flag: {flag}"))?;
        media_key_on(&conn, BUS_NAME, key).await
    });
    match result {
        Ok(()) => 0,
        Err(e) => {
            eprintln!("orange: {flag} failed ({e}); is Orange running?");
            1
        }
    }
}

/// Headless launch: read-only summary of the existing collection.
fn run_headless() {
    println!("{} — Made by {}", orange_app::about::title(), MAKER);
    let data_home = std::env::var("XDG_DATA_HOME").unwrap_or_else(|_| {
        let home = std::env::var("HOME").unwrap_or_else(|_| String::from("~"));
        format!("{home}/.local/share")
    });
    let db_path = identity::collection_db_path(&data_home);
    println!("Collection: {db_path}");
    let path = std::path::Path::new(&db_path);
    if !path.exists() {
        println!("No Orange collection yet. Add music directories in the app to build one.");
        println!("Strawberry data (if any) is left in place, never moved or deleted.");
        return;
    }
    match orange_db::open_collection(path, orange_db::OpenMode::ReadOnly) {
        Ok(conn) => {
            let songs = orange_db::count_rows(&conn, "songs").unwrap_or(-1);
            let playlists = orange_db::count_rows(&conn, "playlists").unwrap_or(-1);
            println!("Songs: {songs}");
            println!("Playlists: {playlists}");
        }
        Err(e) => {
            eprintln!("orange: could not open collection read-only: {e}");
            std::process::exit(1);
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn args(flags: &[&str]) -> Vec<String> {
        flags.iter().map(|flag| flag.to_string()).collect()
    }

    #[test]
    fn parses_every_flag() {
        assert_eq!(parse_args(&args(&["--version"])), Command::Version);
        assert_eq!(parse_args(&args(&["-V"])), Command::Version);
        assert_eq!(parse_args(&args(&["--help"])), Command::Help);
        assert_eq!(parse_args(&args(&["--ui"])), Command::Ui);
        assert_eq!(
            parse_args(&args(&["--play-pause"])),
            Command::MediaKey("--play-pause".to_string())
        );
        assert_eq!(
            parse_args(&args(&["--stop-after-current"])),
            Command::MediaKey("--stop-after-current".to_string())
        );
        assert_eq!(parse_args(&args(&[])), Command::Headless);
    }

    #[test]
    fn serve_collects_files() {
        let command = parse_args(&args(&["--serve", "/music/a.flac", "https://x/y.opus"]));
        let Command::Serve { uris } = command else {
            panic!("expected Serve, got {command:?}");
        };
        assert_eq!(uris.len(), 2);
        assert!(uris[0].starts_with("file://"));
        assert!(uris[0].ends_with("/music/a.flac"));
        assert_eq!(uris[1], "https://x/y.opus");
        assert_eq!(
            parse_args(&args(&["--serve"])),
            Command::Serve { uris: vec![] }
        );
    }

    #[test]
    fn desktop_actions_all_parse() {
        // Every Exec= line in dist/unix/com.goshapps.Orange.desktop.
        for flag in [
            "--play-pause",
            "--stop",
            "--stop-after-current",
            "--previous",
            "--next",
        ] {
            assert!(
                matches!(parse_args(&args(&[flag])), Command::MediaKey(_)),
                "{flag}"
            );
        }
    }
}
