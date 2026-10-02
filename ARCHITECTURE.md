# Orange cross-platform architecture

Orange retains its Rust domain crates and SQLite schema. The canonical window
will use stable Dioxus 0.7 Desktop: WebView2 on Windows, WKWebView on macOS,
WebKitGTK on Linux. GTK is the Linux WebView host dependency, not the UI
implementation. No Electron, Node, React or TypeScript application runtime.

The Dioxus component tree renders immutable snapshots and emits typed
application commands. Menus, keyboard shortcuts, context actions and buttons
share those commands. A dedicated service worker owns collection I/O,
playback, queue sequencing and persistence. It sends bounded snapshots back
to the UI; expensive scans/network/file operations never execute in rendering.
Cancellation and worker shutdown must preserve data and stop owned pipelines.

`orange-core` contains portable paths/URLs and models; `orange-db` owns
transactional compatibility; `orange-collection` scans and filters;
`orange-playlist` owns parsers and queue operations; `orange-media` keeps the
GStreamer audio/DSP and optional integrations. GStreamer is retained because
the existing playback/transcoding tests exercise real behavior and it supports
Windows/macOS/Linux natively. Packaged builds must supply its runtime/plugins.

`orange-app` contains commands/state/services, settings, platform integrations,
and small frontend components. The platform boundary handles native dialogs,
reveal/open operations, native menu conventions and OS capabilities. Remaining
Linux-only MPRIS/UDisks/CD/Discord integrations are opt-in capabilities, not
assumptions in portable UI code. UI labels must reflect real capabilities.

Settings use per-platform OS directories and atomic JSON replacement. Linux
retains Orange's existing paths; useful legacy QSettings appearance/volume
values are imported once without rewriting their source. Queue, custom radio,
theme, equalizer and window state are saved. Invalid settings generate a
diagnostic and are preserved; files are never silently overwritten as repairs.
Existing SQLite schema/data and Strawberry paths retain their protections.

Worker errors are explicit status/error state. Logs record diagnostic context
without credentials or full authenticated URLs. No telemetry is added. User
files change only through explicit import metadata, tag edit, export or sync
commands. File/network inputs use typed PathBuf/URL validation and bounded
parsers; external programs use argument arrays through a capability boundary.

Version 3.1.0-alpha.1 denotes an incomplete migration, not a stable release.
CI checks domain logic on Windows/macOS/Linux and builds each native frontend.
Release automation must gate publishing on successful artifacts and checksum
generation. Flatpak uses GNOME's WebKitGTK-capable SDK/runtime, portal dialogs,
explicit audio/network permissions and hermetic Cargo sources. Runtime QA and
package installation are separate from successful host compilation.

The latest-snapshot mailbox replaces its pending value instead of queuing every audio tick. Library selectors compare shared song vectors so spectrum/position updates do not refilter the collection. Settings writes are debounced; desktop shutdown waits for the command owner to persist settings and stop audio. Equalizer gains update the existing pipeline in place.

An explicit absolute ORANGE_PROFILE_DIR isolates QA on every OS. Runtime plugin paths are discovered only inside actual packaged resources; mutable settings remain in native user directories. The local GLib binding patch is documented in vendor/PATCHES.md.
