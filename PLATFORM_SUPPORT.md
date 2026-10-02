# Platform support and release gates

This is an alpha migration. A platform is verified only to the extent shown
below. There are no fabricated Windows/macOS screenshots or CI success claims.

| Target | Build evidence | Runtime evidence | Packaging evidence | Status |
|---|---|---|---|---|
| Linux x86_64 / Debian 13 | Stable Rust check/build and full tests | Actual WebKitGTK window on X11/Xvfb; WebView interaction and manual theme/resize inspection | Archive recipe and desktop/AppStream validation | PARTIAL: host portal selection and archive verified; physical audio and full native/manual QA pending |
| Linux Wayland | Shared implementation | No real Wayland compositor in this session | Same Linux recipe | NOT VERIFIED |
| Linux aarch64 | Native CI job configured | No ARM machine available | Archive recipe configured | NOT VERIFIED |
| Windows x86_64 | MSVC CI job configured | No Windows machine available | NSIS/ZIP and install/uninstall smoke steps configured | NOT VERIFIED |
| macOS Apple Silicon | Native CI job configured | No macOS machine available | .app/DMG/ZIP with relocation/signature checks configured | NOT VERIFIED |
| macOS Intel | Native CI job configured | No macOS machine available | Same native bundler | NOT VERIFIED |
| Flatpak x86_64/aarch64 | Manifest and source contract validated | No completed sandbox build | Offline GNOME 49 manifest and bundle jobs configured | BLOCKED: Flathub proxy access denied |

## Local evidence

The baseline COSMIC app was built, launched and interacted with before rewriting.
The migrated Dioxus app was launched in the actual native WebView, with an
isolated copy of the collection. The opt-in `ui-qa` harness tests DOM events in
that WebView, not a mocked renderer or a separate browser application. Its native
runs passed search, queue playback/pause/stop/next/previous, repeat selection,
volume, Unicode playlist CRUD, undo/redo, invalid/custom radio stations,
navigation, all appearance choices, About/lyrics dialogs, rating changes and
background rescanning. The final rerun and exact test totals are recorded in
MIGRATION_AUDIT.md.

GStreamer output used a synchronized `fakesink` explicitly selected by
`ORANGE_AUDIO_OUTPUT=null`; this tests decoding/state transitions but does not
prove physical sound. Native folder dialogs opened and cancellation was tested;
host folder selection/import, FLAC save/conversion and two-track copy are verified; sandbox grants require separate verification. The cloud lacks
`/dev/fuse`, and its document portal reports a failed mount. System WebKit helper
paths were exposed from a user-installed native sysroot through a temporary
mount namespace; WebKit's sandbox was not disabled.

Actual Linux light/dark captures are in `docs/screenshots/`; resizing down to
720×560 was inspected with the content panes scrolling and controls reflowing.
The exact extracted release binary also launched at GTK scale 2, with correct
logical-size persistence and minimum-window inspection (see linux-hidpi.png).
Retina, Windows DPI, accessibility/screen-reader behavior and system-theme
changes from a real desktop remain manual QA gates.

## External blockers

The session proxy returned CONNECT 403 for `api.github.com`,
`gstreamer.freedesktop.org`, `flathub.org`/`dl.flathub.org`. Actions HTML is readable and native Git push preflight succeeds; the API remains denied.
[Actions run 36950731642](https://github.com/goshitsarch-eng/Orange/actions/runs/36950731642)
was triggered by commit `e291eb468` on `codex/dioxus-desktop-migration`.
All seven native/Flatpak jobs were refused before startup because GitHub reports:
"The job was not started because your account is locked due to a billing issue."
The run is marked Failure, with no artifacts. This is an account blocker, not
an executed compiler/test failure. Resolve the owner’s GitHub billing lock and
rerun this workflow before claiming native CI or packaging success. Normal Git checkout access is separate
from API/Actions access; no token was requested or substituted. Windows/macOS
runners and native installer testing are required to close those gates.

## Dependency review

Rustls was upgraded to 0.23.45 for RUSTSEC-2026-0285. GLib 0.18's upstream
VariantStrIter pointer fix was backported locally and is covered by an optimized
regression test; see `vendor/PATCHES.md`. The dependency family is constrained by
Dioxus Desktop's current GTK 3 host and GStreamer bindings.

`cargo audit` reports zero known security vulnerabilities. Remaining warnings:
fxhash (RUSTSEC-2025-0057), paste (RUSTSEC-2024-0436), proc-macro-error
(RUSTSEC-2024-0370), and Rand 0.7 (RUSTSEC-2026-0097). Rand is used by build-time
PHF generation in Wry's HTML dependency; Orange's logger does not access its
thread RNG. No advisory is hidden by an ignore list. Upstream replacement of
these dependencies remains a maintenance gate for a long-lived stable release.

## Release checklist

- Run every OS/architecture job; inspect and fix failures rather than accepting YAML syntax as evidence.
- Exercise installed packages on clean native machines, including install/uninstall and file associations.
- Verify physical playback, pause/seek/volume, repeat/end-of-stream and changing tracks.
- Exercise native dialogs, drag/drop, clipboard, URL/reveal actions and portal grants.
- Verify native theme changes, minimum/window sizes, DPI/Retina and screen-reader operation.
- Verify Flatpak offline build, WebKit helpers, portal selection, audio and exact permissions.
- Close all useful-feature parity gaps in MIGRATION_AUDIT.md before retiring legacy source.
- Verify library relocations/plugin licenses, sign/notarize public artifacts where available, and verify SHA-256 sidecars.
