# Orange migration audit

## Flutter rewrite audit of the current checkout

Reference: `eec53d8286870cf8b44eb2aa2efc3eba5d0737ed` on `master`.
This section supersedes the historical migration claims below for the new
Flutter/Dart/Rust request. No feature has yet been migrated to Flutter.
The older inventory remains as evidence to preserve, not proof of current
cross-platform functionality. Target responsibilities and verification gates
are detailed in [the Flutter migration design](docs/flutter-migration.md).

### Current source and architecture

The supplied checkout is not solely a Linux application: its current canonical
frontend is Rust/Dioxus Desktop, alongside retained Strawberry-derived Qt/C++
sources. Neither its README nor previous audit proves Windows/macOS/Flatpak
runtime readiness. The replacement requested here is Flutter, with Dart owning
presentation and appropriate Rust services retained.

`crates/orange-app/src/main.rs` parses version/help/headless/daemon/remote and
file/URI launch commands. `ui/mod.rs` owns the Dioxus window and Signal-based
presentation state; `ui/chrome.rs` owns transport/navigation/menu actions;
`ui/library.rs`, `ui/pages.rs`, and `ui/dialogs.rs` render library, queue,
playlists, radio, files, devices, settings and dialogs. These six UI modules
contain 1,781 lines. `platform/mod.rs` owns RFD file dialogs, Muda menus,
window icons and reveal/open operations.

`service.rs` is a 1,264-line command owner with cancellable background I/O jobs,
queue sequencing, settings persistence and GStreamer lifecycle. It publishes
latest snapshots through an `Arc<Mutex<Option<Snapshot>>>`; consumers take
snapshots rather than accumulate ticks. `commands.rs` defines typed commands.
Presentation drafts and search/expansion fields also exist in Rust dialog and
collection shell models; those must not become the Flutter backend API.

The other seven crates contain portable identity/path/song models, rusqlite
storage, scans/filtering/tree models, playlist formats/undo, smart playlists,
media engines/providers and appearance helpers. Major dependencies include
Dioxus 0.7.10, Wry/WebKitGTK, GTK bindings with the local GLib safety patch,
GStreamer, lofty, rusqlite, Tokio, reqwest/rustls, zbus and notify-rust.
The UI-free domain build does not require WebKit or GTK. Cargo's pinned lockfile
and vendored patch were preserved during audit/setup.

The Qt reference remains in `src/`, `tests/src/`, `CMakeLists.txt`, `dist/` and
`debian/`. It includes advanced cover, device, organizer, streaming, shortcut,
OSD and context workflows not proven equivalent in the current Rust frontend.
The legacy form inventory below accounts for those surfaces; their runtime
behavior remains UNKNOWN in this session. Do not delete them on the strength
of a successful Dioxus smoke test.

### Current run evidence

Verified in this cloud instance on Debian 13 x86_64 with stable Rust 1.99.0:

- Native `orange-app` debug build with `ui-qa` completed.
- Portable workspace tests: **143 passed, 0 failed**.
- Full-feature workspace tests in a private D-Bus session: **177 passed,
  0 failed, 2 explicitly ignored Internet tests**.
- `cargo fmt --all -- --check` and all-target/all-feature Clippy with warnings
  denied passed.
- The independently Python-authored SQLite compatibility fixture passed all
  12 `orange-db` tests; the headless/version CLI also executed successfully.
- The actual native WebView executed **27 successful UI assertions**, plus
  **4 successful playlist-launch assertions** in `scripts/qa/run-linux.sh`.
  This exercised real GStreamer decoding with a null sink, Unicode paths,
  search, queue transport, playlists, undo/redo, repeat, volume, validation,
  radio persistence, file navigation, theme changes, About/lyrics dialogs,
  ratings, rescanning and tag writing. Conversion output selection was rendered;
  this UI script does not perform a complete conversion/save-dialog workflow.

Native dependencies are verified Debian packages extracted under
`/workspace/orange-env/native` because the container is not system root.
Build activation and helpers live outside the checkout. A private mount
namespace overlays only WebKit's missing helper directory. WebKit's own
sandbox remains enabled. An initial wrapper also isolated networking and
produced a blank window/timeouts; keeping existing networking fixed the UI
suite. An initial full test lacked `alsasink`; installing the verified
`gstreamer1.0-alsa` package fixed it. These were setup failures, not confirmed
repository regressions. The complete install script was rerun successfully.

Physical sound, every native menu/shortcut, portal grants, clipboard,
drag/drop, real online providers, Wayland, high-DPI/Retina, Windows, macOS,
installer operation and actual Flatpak runtime remain unverified here.
Previous session screenshots and CI claims below are historical and were
not re-certified by this audit.

### Feature responsibility and Flutter parity inventory

The table below is the **pre-rewrite reference audit**. Its current-state column
records behavior at the audited commit, not the new Flutter implementation.
The former `orange-app` service files now live in `orange-services`.
The current Flutter migration inventory follows this section; historical
reference evidence must not be confused with Flutter certification.

| Feature | Current state / source | Flutter/Dart responsibility | Rust responsibility / remaining gates |
|---|---|---|---|
| CLI and launch files | WORKING: `orange-app/src/main.rs`, native launch QA | Desktop lifecycle/open-file delivery | Preserve CLI/domain parsers; test Finder/Windows associations |
| Window and seven source pages | WORKING on X11: `orange-app/src/ui/` | Widgets, layout, navigation, focus | No widget/window objects in core; native OS QA required |
| Collection add/rescan/remove | PARTIAL: `orange-app/src/library.rs`, `orange-collection/src/scan.rs` | Folder selection, progress, cancellation controls | Transactional scanning; explicit folder permissions; portal tests |
| Genre/artist/album search | WORKING: `orange-app/src/ui/library.rs`, `orange-collection/` | Inputs, selected filters, paged list presentation | Queries/filter rules; batch DTOs rather than whole library on ticks |
| Tags and tag editor | WORKING in WAV QA: `orange-media/src/tagger.rs`, app UI dialogs | Draft fields and user-facing validation | Validate and atomically write tags; test other formats |
| SQLite compatibility, ratings/statistics | WORKING: `orange-db/`, `data/schema/` | Rating interaction | Schema 23, additive migration, identity/statistic retention; external fixture gate |
| Queue add/play/remove/clear | WORKING: `orange-media/src/playback.rs`, app service | Selection, virtual rows, actions | Single command owner and stable cursor/audio agreement |
| Undo/redo queue changes | WORKING: app service, `orange-playlist/src/undo.rs` | Shared keyboard/menu Actions | Queue history rules; edit-field undo stays in Flutter |
| Play/pause/stop/seek/volume | PARTIAL: `orange-media/src/backend_gst.rs` | Accessible transport widgets | GStreamer pipeline; actual speakers and every OS remain gates |
| Repeat/shuffle/stop-after | PARTIAL: playback + app service | Mode controls | Sequencing/one-shot state; end-of-stream UI QA incomplete |
| Equalizer and spectrum | PARTIAL: media DSP and app UI | Sliders and spectrum painting | DSP and live gain changes; physical/runtime QA |
| Waveform/moodbar/normalization | PARTIAL: `orange-media/src/audio_fx.rs`, legacy `src/` | Appropriate visual controls | Retain algorithms; surface and test intended workflows |
| Saved playlists CRUD | WORKING: `orange-db/src/library.rs`, app UI pages | Names/dialogs/list state | Transactional load/save/delete; Unicode preserved |
| Smart playlists | PARTIAL: `orange-smartplaylists/`, app library | Five view choices | Rating/history generators; keep business rules in Rust |
| M3U/PLS/XSPF import/export | WORKING parsers, M3U launch: `orange-playlist/` | Native file dialogs and progress | Checked parsing, path resolution and atomic export; platform paths |
| Preset and custom radio | PARTIAL: `orange-media/src/radio.rs`, app settings | Station management and playback UI | Validate HTTP(S), persist stations and play streams; live network unverified |
| Radio Browser search | PARTIAL: `orange-media/src/net.rs`, app service | Results/loading/error state | Provider engine; actual provider access/test fixtures |
| Files browsing/folder playback | WORKING browser QA: `orange-app/src/files.rs` | Folder controls, native picker, drop UI | Async listing/import; selected-folder grants and URI conversion |
| Copy to selected folder/device | PARTIAL: `orange-media/src/devices.rs`, app service | Destination selection, progress, cancel | Safe names, atomic copies, integrity; native grants/hardware QA |
| Direct MTP/iPod | NOT IMPLEMENTED in current Rust UI: devices models, legacy device code | Verified capability UI only | Actual protocols/hardware required; do not remove legacy reference |
| Audio CD | PARTIAL: `orange-media/src/cd.rs`, legacy sources | Drive/track selection | Linux ioctls and portable TOC abstraction; hardware/OS work required |
| Light/dark/system | WORKING explicit choice: settings + UI CSS | Flutter ThemeData/ThemeMode and OS theme events | Compatible atomic preference persistence; system event tests |
| Preferences and window state | PARTIAL: `orange-app/src/settings.rs` | Preference presentation and window lifecycle | Compatible settings reader/writer; size persistence via plugin |
| About/lyrics dialogs | WORKING rendering: app UI dialogs | Dialogs, identity, accessible layout | Stored lyrics and optional LRCLIB engine; live lookup unverified |
| Covers/AcoustID/MusicBrainz | PARTIAL/UNKNOWN: media network and legacy cover code | Results/art display | Existing provider/parsing engines; authentication/provider tests |
| Audio conversion | PARTIAL: media GStreamer tests + app dialogs/service | Preset/output picker, progress/cancel | Safe conversion/encoders; native save-path and packaged plugins |
| Keyboard/menu/context actions | PARTIAL: app `platform/mod.rs`, UI chrome/dialogs | Actions/Shortcuts, platform menus, focus | Shared domain operations; no Dioxus/Muda dependency in target core |
| Clipboard/reveal/open URL | PARTIAL: app platform code | Flutter clipboard and platform adapters | Validate relevant inputs; avoid universal Linux shell commands |
| Drag/drop | PARTIAL: app UI root | Native drop plugin and selected-file grants | Import/parse paths; Windows/macOS/Flatpak tests required |
| Accessibility/resizing/DPI | PARTIAL: app UI and window builder | Semantics, focus traversal, adaptive layout | No native layout responsibility; OS/screens-reader QA |
| Notifications | PARTIAL: `orange-app/src/notify.rs` | Evaluate reliable desktop plugin and permission lifecycle | Track events; existing notifications are not runtime proof |
| MPRIS/UDisks | PARTIAL: `orange-media/src/mpris*`, `udisks.rs` | Capability/status UI | Opt-in Linux adapters, live private-bus tests; avoid universal assumptions |
| Last.fm/ListenBrainz/Subsonic | PARTIAL: `orange-media/src/net.rs`, `scrobble.rs` | Credential entry + OS secure storage/session presentation | Authentication/provider engine; no fake signed-in claims |
| Spotify/Tidal/Qobuz | UNKNOWN/PARTIAL: media enums, legacy streaming code | Account and playback surfaces after verification | Legacy behavior inventory and working provider integrations |
| Discord presence | PARTIAL: `orange-media/src/discord.rs` | Preference presentation | Unix implementation/Windows named pipes, lifecycle tests |
| Advanced Qt forms/workflows | UNKNOWN: `src/`, form inventory below | Audit/recreate useful workflows | Retain domain behavior; each deferred feature remains a parity gate |
| Identity/assets/desktop metadata | PARTIAL: `data/icons/`, `dist/` | Reuse Orange assets in Flutter runners | Consistent ID/version/license; test installed associations |
| Native packaging and release | UNKNOWN current native targets: `.github/workflows/rust.yaml`, `scripts/package/` | Flutter bundle/runners | Rust library/plugin architecture matching; real CI/package tests |
| Flatpak | UNKNOWN runtime: `data/com.goshapps.Orange.yml` | Flutter bundle, portal integrations | Offline native library/dependencies, least permissions; sandbox tests |

### Concrete defects and risks found in source

`orange-app/src/dialogs.rs` still describes COSMIC dialogs although its models
are not rendered by COSMIC. `ScrobblerAuth::sign_in` transitions to SignedIn
for nonempty local strings without contacting an authentication provider.
It is a model, not evidence of working authentication; the Flutter UI must
never expose this transition as a successful login. This was observed by
source inspection and the existing model test, not a real provider attempt.
The current frontend does not expose a complete account lifecycle.

Rust's platform reveal helper invokes `explorer.exe` or `/usr/bin/open` using
argument arrays; its Linux branch delegates to `open`. Optional fingerprinting
invokes `fpcalc`. Media CD/device adapters include Linux `/dev` and `/proc`
assumptions. Preserve argument safety and isolate those operations; do not
translate them into portable-looking Dart shell strings.

The current worker sends cloned settings and library references in snapshots.
The target bridge must avoid serializing a full library on every playback tick.
Settings currently mix persisted presentation preferences and queue state;
split ownership/API without breaking the existing JSON format. Deleting the
old frontend now would remove behavior before Flutter parity exists.

### Flutter migration progress (2026-10-04)

Flutter 3.47.6 / Dart 3.13.5 installed from the official stable Git tag
`5fc346839b5d0eef006ed8404392afb4dfae428d`. Generated typed bridge 2.13.0 and
native Linux debug/release bundles now build. The original SDK-domain block
was resolved; the official release manifest still returned 404, so the
verified official Git tag and Flutter's own artifact downloader were used.

| Feature | Migration status | Implementation / evidence | Remaining gates |
|---|---|---|---|
| Core shell/navigation/menus/shortcuts | MIGRATED | desktop/lib/main.dart, commands/, ui/chrome.dart; native UI QA | Target OS menus, keyboard/focus/accessibility and DPI |
| Collection/search/filter/smart views | VERIFIED on Linux fixture | services/library.rs, bridge query DTOs, library_page.dart; real scan/Unicode bridge tests and native search UI | Large-library timings, portals and native folder selection |
| Queue/transport/undo/playlist CRUD | VERIFIED on Linux fixture | Rust worker + Flutter queue/catalog pages; native play/pause/stop/Unicode-save/clear/undo | EOS and manual context/menu/shortcut matrix |
| SQLite/settings compatibility | VERIFIED scoped regression | Existing domain tests + real Dart close/reopen test | External live user profiles on each target |
| Tags/copy/import/export | MIGRATED | Domain workers retained; native picker adapters; Rust regressions | Every Flutter dialog/permission workflow, all tag formats |
| Audio conversion | VERIFIED installed targets through real bridge | Eight domain targets; runtime factory availability; actual conversion outputs | Packaged plugins, native output picker, target-specific codecs |
| Themes/window size/resizing | VERIFIED under X11 | Native light/dark/system tests, 600x400/720x560/1280x800 screenshots | OS theme events, Retina/scaling and Wayland |
| EQ/spectrum/ratings/repeat/shuffle/stop-after | MIGRATED | Flutter sliders/painter and typed commands; existing DSP/sequencing regressions | Physical audio and target-specific DSP QA |
| Radio/lyrics/files/device-folder copy | MIGRATED | Flutter catalog pages; existing Rust service/provider jobs | Live providers, picker grants and removable hardware |
| CLI/headless/MPRIS | MIGRATED | orange-cli bundled independently of UI; CLI and bus regressions | Desktop actions/install and remote live-instance QA |
| Notifications | MIGRATED adapter | Existing notify-rust adapter retained in native bridge feature | Daemon/platform delivery and permissions |
| Native dialogs/clipboard/drop/reveal | MIGRATED adapters | Official file_selector/url_launcher; window_manager/desktop_drop; Flutter edit conventions | Manual OS/Flatpak grants, already-running Finder open events |
| Linux release archive | BUILT | Flutter bundle, native dependency closure, GStreamer/plugins/scanner, archive/checksum | VERIFIED without SDK paths: startup, Rust database, Ctrl+Q and persisted settings; other distribution baselines pending |
| Windows/macOS packaging | IMPLEMENTED, UNVERIFIED | Native runner hooks; NSIS/ZIP; dylib relocation/ad-hoc signing/DMG/ZIP | Actual target builds, package installs and manual QA |
| Flatpak | IMPLEMENTED, BLOCKED | Offline SDK/Pub/Cargo preparation, revised manifest and source generation | dl.flathub.org returns HTTP 403; runtime/encoder/portal install QA |
| Remote CI/release | IMPLEMENTED, UNVERIFIED | Flutter target matrix; generated-binding, security, package/checksum gates | GitHub CLI access now works; no migration CI run or release published |
| Advanced Qt/account/CD/MTP/iPod workflows | NOT IMPLEMENTED in Flutter | Reference inventory and protocol/domain sources retained | Audit intended live behavior, credentials/hardware and cross-platform implementation |

Regression results after extraction: **149 portable Rust tests passed**;
**183 full-feature Rust tests passed**, two Internet tests ignored;
Dart/real-native-bridge/widget tests passed (5 tests), including installed
conversion targets. Native Flutter UI QA passed the fixture workflow and
resize/theme checks, seek/volume/repeat/shuffle/equalizer controls. The extracted release archive starts without SDK paths and exits through Ctrl+Q with saved settings (empty-profile startup 0.302 s; idle RSS 173276 KiB in this container). Full-workspace Clippy denies warnings and passes.
`cargo audit` 0.22.2 reports no blocking vulnerabilities and four warnings:
legacy-reference fxhash and rand 0.7.3, and compile-time paste/proc-macro-error
through GStreamer/Lofty/GLib. The first two are absent from the Flutter bridge's
native dependency tree. Retained GLib safety patch remains necessary for the
old bindings; upgrading the media binding stack remains maintenance work.

New fixes: strict credential-free HTTP(S) radio validation; owned background
jobs joined on shutdown; runtime AAC encoder fallback; conversion dialog uses
the domain capability table rather than invented/missing formats; bounded
pages/revision polling; compact minimum-size library layout; independent
row action hit targets and keyboard focus for application shortcuts; corrected repeat-mode value labels.

The migration is **not production-complete**. Physical speakers, Wayland,
Windows/macOS/Flatpak, live providers, file grants and retained advanced
features remain open gates. Keep reference sources until those useful
workflows have verified equivalents. The current release scripts and CI
must not ship the reference frontends alongside Flutter.

## Historical COSMIC-to-Dioxus migration record

Baseline: `f5bf20195` (Orange 3.0.0). Audit begun before replacing the frontend.
This is a living inventory, not a declaration of cross-platform release readiness.

## Evidence and reference

The baseline was built and launched on Debian x86_64 in Xvfb. Its 126 default
tests and 160 full-feature tests passed; two explicitly ignored Internet tests
were not executed. Real GStreamer fake-sink playback, transcoding, tag I/O,
SQLite compatibility and MPRIS round-trips ran. The window, sidebar pages,
import workflow, menu and resize behavior were exercised with native mouse
and keyboard events. A folder containing Unicode paths and two WAV files was
indexed; `--headless` then reported two songs. Physical playback, hardware,
authenticated services and the older Qt executable were not verified.
Reference executable and captures are retained outside the checkout in
`/workspace/.orange-migration/reference`; source history remains in Git.
The current documentation is not accepted as proof of behavior.

## Architecture at baseline

Two generations coexist. `src/`, CMake, `tests/src/`, `dist/`, and `debian/`
contain the Strawberry-derived C++17/Qt6 application. `Cargo.toml` defines
eight Rust crates: core identity/song/path models; SQLite database; collection
query/scan/tree; playlists/parsers/undo; smart playlists; media/audio/network;
theme; and application. The current Rust executable is `orange-app/src/main.rs`.
`orange-app/src/ui.rs` is a roughly 1,500-line COSMIC component owning the
player, collection, engine, navigation, forms, settings and presentation.

Core dependencies include rusqlite (bundled SQLite), GStreamer, lofty, reqwest
with rustls, Tokio, zbus, notify-rust, and git-pinned libcosmic/iced. Most Rust
logic is already reusable without a UI. The Qt application also uses TagLib,
Boost, ICU, KDE-style integration and many optional device libraries.

Data: Orange's existing SQLite schema version 23 (the compatibility fixture begins at 22), directories, songs,
playlists and playlist_items; M3U, PLS, XSPF playlist helpers; audio tags;
Qt INI/QSettings configuration. Linux Orange paths are
`$XDG_DATA_HOME/orange/orange/orange.db` and
`$XDG_CONFIG_HOME/orange/orange.conf`. Strawberry data must never be moved,
deleted or automatically migrated. Rust theme/radio/queue state was volatile.

Platform assumptions include XDG-only home discovery, manually encoded file
URLs, `/proc/mounts`, `/dev/sr0`, Linux CD ioctls, UDisks2/MPRIS, Unix Discord
sockets and system icon directories. The only Rust external program found
outside tests is optional Chromaprint `fpcalc`; it uses argument arrays,
not a shell. Qt has additional QProcess/file-manager and desktop services.
Linux-specific integrations must be isolated; capability claims require
actual implementations and platform evidence.

Packaging: Rust CI tests default and full COSMIC builds; legacy CI has Linux,
Windows and macOS Qt jobs. Flatpak 25.08 uses a generated Cargo source list
and requests home, all-device and wildcard MPRIS permissions. Those broad
permissions need justification or removal. Existing source/vendor lists must
be regenerated after dependency changes. No working Dioxus packages exist
at baseline.

## Feature inventory

Current states describe baseline source/runtime. Migration entries describe
3.1.0-alpha.1. MIGRATED/IMPROVED is scoped to Linux/domain evidence below;
PARTIAL and DEFERRED explicitly block completion, with legacy implementations
retained. None implies Windows/macOS/Flatpak runtime certification. No useful
feature is declared intentionally removed without a justification.

| Feature | Current state | Source at baseline | Expected behavior / known defects | Migration | Platform considerations |
|---|---|---|---|---|---|
| Version/help/headless CLI | WORKING | app/main.rs | Read-only counts; unknown flags silently ignored | IMPROVED: preserved commands, unknown options fail; native paths; read-only archive smoke | Native paths and file URLs |
| Native window/navigation | PARTIAL | app/ui.rs | Music, queue, radio, files, devices; system icons disappear | MIGRATED: Dioxus Desktop components, bundled transport icons; actual WebView QA | One Dioxus frontend, OS WebViews |
| Collection import/rescan/remove | PARTIAL | app/library.rs, collection/scan.rs | Synchronous scans block UI; failed scans can erase indexed rows | IMPROVED: cancellable worker jobs; checked scans; native import/removal verified | Portals, Unicode, inaccessible folders |
| Genre/artist/album browser/search | WORKING | app/library.rs, collection/filter.rs | Preserve search fields and grouping | MIGRATED: shared selectors and paged rows; search QA; portable filter tests | Portable domain logic |
| Audio tags on scan | WORKING | media/tagger.rs | lofty reads supported formats | MIGRATED: same lofty backend; Unicode tag write/rescan QA | No TagLib dependency |
| SQLite read compatibility | WORKING | db/lib.rs, data/schema | Schema 22 fixture; refuse Strawberry writes | MIGRATED: existing migrations retained; 12 DB/compat tests, CPython oracle | Preserve per-user data |
| Rescan statistics/ratings | BROKEN | db/library.rs | Delete/reinsert loses user play/rating metadata | IMPROVED: transactional ROWID/statistics/fractional-rating retention; regression + UI QA | Transactional updates |
| Queue play/add/remove/clear | PARTIAL | media/playback.rs, app/ui.rs | Active remove does not resync engine | IMPROVED: one command owner resyncs engine; undo/redo; WebView QA | Stable cursor/engine agreement |
| Playback/pause/stop/seek/volume | WORKING | media/backend_gst.rs | Real test pipelines; physical output unverified | MIGRATED: real decoding/transcode pipelines; WebView transport; physical audio pending | GStreamer supports native sinks on all OSes |
| Repeat/shuffle | PARTIAL | app/ui.rs, playlist/model.rs | UI toggles modes; shuffle and track/album repeat not applied | IMPROVED: sequencing actually applies all modes; stable shuffle; domain regressions; repeat UI QA | Portable sequencing |
| Stop after current | WORKING | media/playback.rs, app/mpris_host.rs | One-shot end behavior and CLI remote | MIGRATED: shared command and one-shot sequencing; unit coverage; end-of-stream UI QA pending | MPRIS is Linux-only |
| Ten-band equalizer | PARTIAL | app/ui.rs, media/audio_fx.rs | Rebuild resets playback position; not persisted | IMPROVED: live pipeline gains, atomic persistence; setting UI/restart verified | DSP backend, all platforms |
| Spectrum/normalization/waveform | PARTIAL | media/audio_fx.rs, backend_gst.rs | Backend helpers; waveform/moodbar not fully surfaced | PARTIAL: live spectrum rendered; normalization/waveform/moodbar backends retained, controls deferred | Preserve retained backend code |
| Saved playlists | PARTIAL | db/library.rs, app/ui.rs | Read existing lists; create/delete not wired | IMPROVED: transactional create/delete/load; Unicode CRUD and undo/redo UI QA | Transactions, import/export |
| Playlist add/remove buttons | DEAD | app/ui.rs:639 | Remove maps to Noop; add opens music import | IMPROVED: rendered Save/Delete controls dispatch real DB jobs; UI QA | Commands shared by menus/buttons |
| Smart playlists | WORKING | app/library.rs, smartplaylists/ | Top rated/recent/never/most played helpers | MIGRATED: retained generators; all five views; ratings/play statistics persist; domain tests | Restore statistics updates |
| Playlist parsers/undo | PARTIAL | playlist/parsers.rs, undo.rs | Library-only; XSPF parser line-based | IMPROVED: native import/export and launch entry points; checked XSPF, BOM/CRLF M3U; parser/atomic-export tests | CRLF, URLs, invalid XML, path resolution |
| Radio Paradise/SomaFM | WORKING | media/radio.rs, app/ui.rs | Station list and play commands; live network unverified | MIGRATED: real presets and play commands retained; live networking unverified | Explicit networking permission |
| Custom radio | PARTIAL | app/ui.rs | In-memory only, lost on restart | IMPROVED: validated HTTP(S), visible errors, saved stations; WebView/restart QA | Persist validated HTTP(S) URLs |
| Files browser/play folder | WORKING | app/files.rs | Custom filesystem browser, no native open dialog | MIGRATED: native dialogs, async folder browser/up/audio rows, folder play and drop; Unicode navigation QA | Native/portal dialogs and drop |
| Theme light/dark/system | PARTIAL | theme/lib.rs, app/ui.rs | Mode not saved; defaults override OS behavior | IMPROVED: persisted choices, CSS system media query; actual light/dark/small-window captures; native OS theme-change QA pending | WebView system theme + settings |
| About/preferences/lyrics pane | PARTIAL | app/ui.rs | About routes to settings; only stored lyrics shown | MIGRATED: actual dialogs, retained identity/stored lyrics; optional LRCLIB; WebView QA; live lookup pending | Accessible dialogs, online lookup optional |
| Keyboard shortcuts/menus | PARTIAL | app/ui.rs | No centralized keyboard command system | PARTIAL: native Muda menus, OS modifier boundary, editable clipboard conventions; every shortcut/menu not yet certified | Ctrl vs Command; native app menu |
| Window state/high DPI | UNKNOWN | app/ui.rs | Fixed assumptions; no saved size | PARTIAL: saved logical dimensions; actual release GUI at scale 2 and minimum 600×400/720×560 logical sizes verified; Retina/Windows DPI pending | OS lifecycle/scale, resize QA |
| Tag editor dialog | NOT IMPLEMENTED | app/dialogs.rs, media/tagger.rs | Validation model and writer exist, no rendered editor | IMPROVED: rendered editor, atomic writer and validation; actual Unicode WAV tag write/rescan QA | Native file access, explicit edits |
| Transcode dialog | NOT IMPLEMENTED | app/dialogs.rs, media/backend_gst.rs | Real converter exists, no rendered workflow | IMPROVED: real format/output/cancel workflow; FLAC/MP3 backend regressions; actual native Save dialog produced valid FLAC output from a disposable WAV | Runtime encoders and safe output |
| USB copy/sync | PARTIAL | media/devices.rs | Real executor; Devices page only claims support | MIGRATED: selected-folder copy with safe names/atomic cancellation; actual portal copy of two tracks matched source SHA-256; direct hardware protocols deferred | User-selected destination; no false hardware claims |
| MTP/iPod sync | NOT IMPLEMENTED | media/devices.rs, app/ui.rs | Labels/models do not implement protocol transport | DEFERRED: no direct protocol transport implemented; misleading support labels removed; mounted-folder copy remains | Needs verified native libraries/hardware |
| Audio CD | PARTIAL | media/cd.rs, backend_gst.rs | Linux ioctl and element construction; no drive QA | DEFERRED in desktop: Linux ioctl/backend retained behind capability; Windows/macOS TOC and drive QA required | OS-specific TOC and plugins |
| MPRIS remote control | WORKING | media/mpris_*.rs | Live round-trip and daemon verified | PARTIAL: live private-bus round-trip verified; interfaces registered before publishing; commands shared with desktop; change signals deferred | Linux optional integration, portable command mapping |
| Notifications | PARTIAL | app/notify.rs | Feature implementation; no desktop notification QA | PARTIAL: best-effort asynchronous native notifications on track change; permission/runtime appearance QA pending | Per-OS permissions, async errors |
| Lyrics/covers/online lookup | PARTIAL | media/net.rs, online.rs | Real HTTP clients plus provider-name models | PARTIAL: LRCLIB/radio UI and real HTTP clients; remaining covers/AcoustID workflows deferred | Credentials/network/fixtures; no invented support |
| Last.fm/ListenBrainz/Subsonic | PARTIAL | media/net.rs, scrobble.rs | Submit helpers, no complete UI auth lifecycle | DEFERRED in desktop: HTTP helpers retained; OS credential storage/auth/session lifecycle required | OS credential storage before exposing accounts |
| Spotify/Tidal/Qobuz | PARTIAL | media/online.rs, Qt streaming/ | Rust enum/model claims only; Qt has implementations | DEFERRED: legacy Qt implementations and Rust provider models retained; complete account/streaming migration required | Accounts and provider access required |
| Discord Rich Presence | PARTIAL | media/discord.rs | Unix socket helper, not UI-integrated | DEFERRED in desktop: Unix helper retained; Windows named pipes and enabled-setting lifecycle required | Windows named-pipe equivalent needed |
| Desktop metadata/icons | WORKING | dist/unix/, data/icons/ | Catalog identity com.goshapps.Orange | MIGRATED: Linux validation passes; Windows resources/macOS identity recipes; native packaging validation pending | Windows resources, macOS bundle metadata |
| Flatpak | UNKNOWN | data/com.goshapps.Orange.yml | Broad permissions; actual sandbox not yet tested | PARTIAL/BLOCKED: GNOME 49 offline manifest, restricted permissions and 630 checksum-matched sources; Flathub denied | Portals, WebKit subprocesses, offline sources |
| Qt advanced features | UNKNOWN | src/{covermanager,lyrics,device,organize,streaming,globalshortcuts,osd,context,...} | Legacy implementation retained; no comprehensive runtime parity proof | DEFERRED: source/forms retained; additional static inventory below; Qt runtime parity not established | Do not remove until feature parity is demonstrated |

## Confirmed defects and guardrails

1. Icon-only transport/menu/playlist controls can be invisible with available
   icon themes. Bundle icons and always provide names/tooltips.
2. Scanning and filesystem operations run on the UI thread. A failed/truncated
   directory walk must not replace a complete collection.
3. Rescans delete/reinsert songs and reset statistics/ratings.
4. Shuffle, track/album repeat, playlist creation/removal and advertised device
   transports are not complete; do not copy their misleading support labels.
5. Settings, streams and queue are not persisted by the Rust UI.
6. `Secret` is a redacted in-memory string, not an OS keyring. Do not describe
   authentication as securely persisted or complete.
7. Manual file URL/path handling is incompatible with Windows/UNC paths and
   duplicated sync decoding mishandles percent-encoded names.
8. Collection reload silently substitutes empty lists on SQL errors.

## Migration verification checklist

For each inventory row record implementation, tests, visible control QA,
Windows/macOS/Linux/Flatpak results, documentation and known limitations.
Compilation is not platform runtime verification. Keep legacy references
until all useful behavior has been accounted for. No stable release claim is
allowed while required platform/runtime/package checks remain unverified.

## Results

Audit complete for current Rust source and the exercised local workflows;
older Qt runtime and authenticated/hardware integrations remain UNKNOWN.
The Dioxus implementation is an alpha preview. Advanced parity and native platform gates remain open; legacy source has not been retired.

## Additional legacy Qt inventory

Static inventory of every `.ui` form below supplements the Rust baseline.
These files were read as an inventory, not executed. Similar domain names do
not establish full parity. Settings not listed as migrated remain DEFERRED:
advanced playback/backend options, network proxy, behavior/context/OSD,
provider ordering, notifications, global shortcut capture, playlist display and
sequence choices, custom collection groupings, cover management/export,
organize/rename workflows, dynamic smart-playlist wizard/preview, advanced
codec parameters and conversion logs, authenticated streaming collections and
search, account/login dialogs, device properties and hardware transports.

These gaps need runtime/product decisions and implementations before the user’s
production definition of done is met. No legacy form is deleted by this change.

| Legacy form group | Forms | Desktop migration / remaining work |
|---|---|---|
| `collection` | `collectionfilterwidget`, `collectionviewcontainer`, `groupbydialog`, `savedgroupingmanager` | Import/search/filter rebuilt; saved grouping manager/grouping choices deferred |
| `core` | `mainwindow` | Main window rebuilt; advanced actions require parity review |
| `covermanager` | `albumcoverexport`, `albumcovermanager`, `albumcoversearcher`, `coverfromurldialog`, `coversearchstatisticsdialog` | DEFERRED: retained Qt implementation; runtime parity unverified |
| `device` | `deviceproperties`, `deviceviewcontainer` | DEFERRED: retained Qt implementation; runtime parity unverified |
| `dialogs` | `aboutdialog`, `addstreamdialog`, `console`, `edittagdialog`, `errordialog`, `messagedialog`, `saveplaylistsdialog`, `trackselectiondialog`, `userpassdialog` | About/errors/streams/tag edits/save playlists rebuilt; login/console/selection extras deferred |
| `equalizer` | `equalizer`, `equalizerslider` | Ten-band gains rebuilt; preset parity deferred |
| `fileview` | `fileview` | Native dialogs and filesystem folder navigation rebuilt; advanced Qt parity review pending |
| `globalshortcuts` | `globalshortcutgrabber` | DEFERRED: retained Qt implementation; runtime parity unverified |
| `organize` | `organizedialog`, `organizeerrordialog` | DEFERRED: retained Qt implementation; runtime parity unverified |
| `osd` | `osdpretty` | DEFERRED: retained Qt implementation; runtime parity unverified |
| `playlist` | `dynamicplaylistcontrols`, `playlistcontainer`, `playlistlistcontainer`, `playlistsaveoptionsdialog`, `playlistsequence` | Saved queues and repeat/shuffle rebuilt; dynamic playlists/options deferred |
| `queue` | `queueview` | Queue controls rebuilt; advanced Qt queue workflows unverified |
| `radios` | `radiobrowsersearchview`, `radioviewcontainer` | DEFERRED: retained Qt implementation; runtime parity unverified |
| `settings` | `appearancesettingspage`, `backendsettingspage`, `behavioursettingspage`, `collectionsettingspage`, `contextsettingspage`, `coverssettingspage`, `globalshortcutssettingspage`, `lyricssettingspage`, `moodbarsettingspage`, `networkproxysettingspage`, `notificationssettingspage`, `playlistsettingspage`, `qobuzsettingspage`, `radiosettingspage`, `scrobblersettingspage`, `settingsdialog`, `spotifysettingspage`, `subsonicsettingspage`, `tidalsettingspage`, `transcodersettingspage`, `waveformsettingspage` | Appearance/volume/music folders/EQ rebuilt; other settings deferred |
| `smartplaylists` | `smartplaylistquerysearchpage`, `smartplaylistquerysortpage`, `smartplaylistsearchpreview`, `smartplaylistsearchtermwidget`, `smartplaylistsviewcontainer`, `smartplaylistwizardfinishpage` | DEFERRED: retained Qt implementation; runtime parity unverified |
| `streaming` | `streamingcollectionviewcontainer`, `streamingsearchview`, `streamingtabsview` | DEFERRED: retained Qt implementation; runtime parity unverified |
| `transcoder` | `transcodedialog`, `transcodelogdialog`, `transcoderoptionsaac`, `transcoderoptionsasf`, `transcoderoptionsdialog`, `transcoderoptionsflac`, `transcoderoptionsmp3`, `transcoderoptionsopus`, `transcoderoptionsspeex`, `transcoderoptionsvorbis`, `transcoderoptionswavpack` | Conversion targets/output/cancel rebuilt; per-codec options/log dialog deferred |
| `widgets` | `loginstatewidget`, `trackslider` | DEFERRED: retained Qt implementation; runtime parity unverified |

## Verified Linux results (2026-10-02)

- Stable Rust 1.99.0, Debian 13 x86_64, GTK 3.24.49, WebKitGTK 2.54.0,
  GStreamer 1.26.2. Default and optimized canonical app builds complete.
- `cargo fmt --all -- --check` and Clippy on the whole workspace/all
  targets/all features with `-D warnings` pass.
- Full-feature suite: **177 passed, 0 failed, 2 ignored**. A private D-Bus
  session ensures the MPRIS round-trip actually executes. The two ignored
  tests require live radio/lyrics Internet services and were not run.
- Domain-only suite: **143 passed, 0 failed**. SQLite suite against the
  independent CPython schema-22 fixture: **12 passed, 0 failed**.
- Optimized GLib iterator regression: **1 passed**. Real GStreamer pipelines
  cover fake-sink playback, decode, FLAC/MP3 conversion and tag I/O.
- Actual WebKitGTK QA uses the opt-in desktop harness and isolated Unicode
  WAV fixtures. Search, queue controls, repeat, volume, saved playlist CRUD,
  undo/redo, radio validation/persistence, navigation, themes, dialogs,
  rating/rescan retention and real tag writes have passed. The final run passed **27 assertions**, including Unicode folder browsing and
  initial rating/repeat selection. A separate native playlist-launch test passed
  **4 assertions** for queue expansion, Unicode titles, untitled entry fallback and
  absence of parser/audio errors. Exact final assertions are saved under `docs/validation/`.
- Native conversion through the rendered dialog and OS Save picker produced
  a real FLAC file from the disposable WAV without modifying the source.
- Native queue copy through the selected-folder portal completed for two tracks;
  every copied SHA-256 matched the originals. An initial read-only mount in the
  cloud launcher was corrected without changing application behavior.
- Native folder import selected the checkout through the actual portal dialog
  and indexed 11 fixture tracks. Removing that directory retained the
  original two-song collection and left source files intact. Native cancellation
  also works. This is host portal evidence, not a Flatpak sandbox proof.
- Linux release archive/checksum generated; SHA-256 verified after extraction.
  The extracted binary reports the alpha version and reads the actual isolated
  collection/playlist counts. Archives require documented system native libraries.
- Exact extracted release binary SHA-256 matches `target/release/orange`. Its
  actual native window was launched at GTK scale 2: 2200×1480 physical pixels
  for 1100×740 logical pixels. Resizing to 1440×1120 correctly saved 720×560
  logical pixels, and minimum 600×400 logical layout was visually inspected.
  This is Linux/X11 scale evidence, not a Windows DPI or Retina certification.
- Dependency audit: zero known vulnerabilities; four transitive maintenance/
  conditional soundness warnings remain documented in PLATFORM_SUPPORT.md.
  All **630** registry-package SHA-256 values match the regenerated Flatpak
  source list and locked originals; no expected checksum was replaced.

The earlier MPRIS test could hang while creating a property proxy. Readiness
now registers all interfaces before publishing the bus name, and the live test
bounds each asynchronous stage. The full private-bus suite passes after this
change. This does not implement the still-deferred PropertiesChanged signals.

Windows/macOS installers, physical audio, Wayland/Retina/screen readers,
Flathub sandbox execution, authenticated services and advanced Qt parity remain
release gates. Configured workflows are not evidence of successful jobs. No
stable release has been published and no reference implementation retired.

## Remote CI attempt

The alpha branch was pushed normally through the configured Git proxy.
[Actions run 36950731642](https://github.com/goshitsarch-eng/Orange/actions/runs/36950731642)
accepted the workflow but refused all five desktop and two Flatpak jobs before
startup: **"The job was not started because your account is locked due to a
billing issue."** No native build, test, installer or Flatpak step executed and
no remote artifact was generated. Publish was correctly skipped on a branch.
This account issue must be resolved by the repository owner; workflow changes
cannot remove it. No CI success is claimed.

Workflow structure and action/shell expressions pass actionlint 1.7.12. This is local lint evidence, not a remotely executed CI run.
