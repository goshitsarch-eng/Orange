//! Native desktop notifications (feature `notify`).
//!
//! Best-effort: every call degrades to `false` when no notification daemon
//! answers (headless CI, minimal sessions). Nothing here blocks playback.

/// Notify about the current track. Returns true when handed to a daemon.
pub fn notify_track(title: &str, artist: &str, album: &str) -> bool {
    let body = match (artist.is_empty(), album.is_empty()) {
        (true, true) => String::new(),
        (false, true) => artist.to_string(),
        (true, false) => album.to_string(),
        (false, false) => format!("{artist} — {album}"),
    };
    show("Orange", title, &body)
}

/// Notify with an explicit summary/body pair (sync reports, errors surfaced
/// to the user, device ejection).
pub fn notify_message(summary: &str, body: &str) -> bool {
    show("Orange", summary, body)
}

fn show(app: &str, summary: &str, body: &str) -> bool {
    notify_rust::Notification::new()
        .appname(app)
        .summary(summary)
        .body(body)
        .icon("com.goshapps.Orange")
        .show()
        .is_ok()
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn notify_never_panics_without_daemon() {
        // Best-effort by contract: the return value reflects daemon
        // presence, but the call itself must never fail the suite.
        let _ = notify_track("So What", "Miles Davis", "Kind of Blue");
        let _ = notify_message("Sync finished", "12 tracks copied.");
    }
}
