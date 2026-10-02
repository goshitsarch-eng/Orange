# Orange migration audit

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
| Playlist parsers/undo | PARTIAL | playlist/parsers.rs, undo.rs | Library-only; XSPF parser line-based | IMPROVED: native import/export entry points; checked XSPF, BOM/CRLF M3U; parser/atomic-export tests | CRLF, URLs, invalid XML, path resolution |
| Radio Paradise/SomaFM | WORKING | media/radio.rs, app/ui.rs | Station list and play commands; live network unverified | MIGRATED: real presets and play commands retained; live networking unverified | Explicit networking permission |
| Custom radio | PARTIAL | app/ui.rs | In-memory only, lost on restart | IMPROVED: validated HTTP(S), visible errors, saved stations; WebView/restart QA | Persist validated HTTP(S) URLs |
| Files browser/play folder | WORKING | app/files.rs | Custom filesystem browser, no native open dialog | MIGRATED: native dialogs, async folder browser/up/audio rows, folder play and drop; Unicode navigation QA | Native/portal dialogs and drop |
| Theme light/dark/system | PARTIAL | theme/lib.rs, app/ui.rs | Mode not saved; defaults override OS behavior | IMPROVED: persisted choices, CSS system media query; actual light/dark/small-window captures; native OS theme-change QA pending | WebView system theme + settings |
| About/preferences/lyrics pane | PARTIAL | app/ui.rs | About routes to settings; only stored lyrics shown | MIGRATED: actual dialogs, retained identity/stored lyrics; optional LRCLIB; WebView QA; live lookup pending | Accessible dialogs, online lookup optional |
| Keyboard shortcuts/menus | PARTIAL | app/ui.rs | No centralized keyboard command system | PARTIAL: native Muda menus, OS modifier boundary, editable clipboard conventions; every shortcut/menu not yet certified | Ctrl vs Command; native app menu |
| Window state/high DPI | PARTIAL: saved logical dimensions and scale handling; X11 resize verified; Retina/Windows DPI pending | app/ui.rs | Fixed assumptions; no saved size | UNKNOWN | OS lifecycle/scale, resize QA |
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
| Flatpak | PARTIAL/BLOCKED: GNOME 49 offline manifest, restricted permissions and 630 checksum-matched sources; Flathub denied | data/com.goshapps.Orange.yml | Broad permissions; actual sandbox not yet tested | UNKNOWN | Portals, WebKit subprocesses, offline sources |
| Qt advanced features | DEFERRED: source/forms retained; additional static inventory below; Qt runtime parity not established | src/{covermanager,lyrics,device,organize,streaming,globalshortcuts,osd,context,...} | Legacy implementation retained; no comprehensive runtime parity proof | UNKNOWN | Do not remove until feature parity is demonstrated |

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
- Full-feature suite: **175 passed, 0 failed, 2 ignored**. A private D-Bus
  session ensures the MPRIS round-trip actually executes. The two ignored
  tests require live radio/lyrics Internet services and were not run.
- Domain-only suite: **141 passed, 0 failed**. SQLite suite against the
  independent CPython schema-22 fixture: **12 passed, 0 failed**.
- Optimized GLib iterator regression: **1 passed**. Real GStreamer pipelines
  cover fake-sink playback, decode, FLAC/MP3 conversion and tag I/O.
- Actual WebKitGTK QA uses the opt-in desktop harness and isolated Unicode
  WAV fixtures. Search, queue controls, repeat, volume, saved playlist CRUD,
  undo/redo, radio validation/persistence, navigation, themes, dialogs,
  rating/rescan retention and real tag writes have passed. The final run passed **27 assertions**, including Unicode folder browsing and
  initial rating/repeat selection. Exact final assertions are saved under `docs/validation/`.
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
