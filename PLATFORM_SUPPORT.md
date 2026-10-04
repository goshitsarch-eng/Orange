# Platform verification

Evidence from the cloud workspace, 2026-10-04. **Implemented does not mean certified.** All Windows/macOS/Flatpak statuses below require target execution. The Linux desktop tests use X11/Xvfb and GStreamer null output; they do not prove physical audio or Wayland behavior.

| Workflow | Linux x86_64 | Windows x86_64 | macOS arm64/Intel | Flatpak x86_64/ARM |
|---|---|---|---|---|
| Flutter window, search, queue, playlists | Native UI tests passed | Runner implemented; untested | Runner implemented; untested | Manifest implemented; blocked |
| Rust domain/storage/bridge | Native and portable tests passed | Target tests pending | Target tests pending | Sandbox tests pending |
| Play/pause/stop, undo, themes, resize | Native UI tests passed | Pending | Pending | Pending |
| Scanning, Unicode data, settings restart | Real Dart–Rust tests passed | Pending | Pending | Portal grants pending |
| Tags, copy, playlist formats | Rust regression tests passed; picker QA pending | Pending | Pending | Pending |
| Eight conversion targets | Real bridge test exercises installed targets; missing factories disabled | Plugin/runtime QA pending | Plugin/runtime QA pending | Build encoder gate; not executed |
| Spectrum, EQ, seek, repeat/shuffle | Native UI and Rust engine tests passed; physical audio pending | Pending | Pending | Pending |
| Native dialogs, clipboard, drag/drop | Plugin integration implemented; manual target QA pending | Pending | Pending | Portals implemented; not executed |
| MPRIS/CLI | Linux bus/core tests; CLI bundled | Headless helper; MPRIS Linux only | Headless helper; MPRIS Linux only | Bus permissions require sandbox QA |
| Notifications | Existing Rust adapter enabled; daemon delivery pending | Pending | Pending | Pending |
| Radio Browser, lyrics/provider Internet | Implemented; live provider checks pending | Pending | Pending | Network permission required |
| Standalone distribution | Extracted archive startup/database/Ctrl+Q/settings passed without SDK paths | NSIS/ZIP automation unverified | DMG/ZIP/ad-hoc automation unverified | Runtime download denied by current network policy |
| Hardware/account/advanced Qt workflows | Parity gates open | Parity gates open | Parity gates open | Parity gates open |

The archive built here uses Debian 13 / glibc 2.41. It must not be advertised as working on older distributions. CI builds Ubuntu 24.04 binaries to establish an older baseline, but that CI has not been executed for these changes. Host graphics drivers remain required. Both native architectures are separate macOS packages; universal GStreamer/Rust assembly is not certified.

The initial setup could not reach the GitHub API or dl.flathub.org. GitHub repository and PR access now works through the configured CLI; Flatpak runtime installation remains unverified. The additional domains are saved in the environment configuration draft, which requires publication before that policy can change. No remote CI run for this migration, Flatpak installation or release publication has been verified.

Release gates: execute the complete CI matrix; install/launch packaged artifacts; validate portals, long/Unicode paths, native picker grants, file associations, keyboard/focus/screen reader behavior, DPI/Retina, system theme changes, real audio, streaming and required hardware/account workflows. Resolve the migration inventory before removing references or calling the rewrite production-ready.
