# :tangerine: Orange Music Player

Orange is a **Qt 6 music player and collection organizer** maintained by Gosh, forked from [Strawberry](https://github.com/strawberrymusicplayer/strawberry), which was forked from Clementine. It prefers Breeze widgets and icons when available and supports system, light and dark color schemes.

**Current release:** 2.1.6

**Maker:** Gosh

The permanent desktop and AppStream catalog identity is `com.goshapps.Orange`. This catalog identity does not rename Orange's existing application name, organization name, QSettings keys, library/database, configuration, or cache locations. Existing Strawberry data is left in place.

## Downloads and support

- [Orange repository](https://github.com/goshitsarch-eng/Orange)
- [Orange releases](https://github.com/goshitsarch-eng/Orange/releases)
- [Report an Orange bug or request a feature](https://github.com/goshitsarch-eng/Orange/issues)
- [Changelog](Changelog)
- [2.1.6 GUI audit and verification](docs/gui-audit-2.1.6.md)

Please include your Orange version, operating system, Qt version, installation method and reproduction steps when reporting an issue. Strawberry's wiki can help with inherited features, but its packages, sponsorship terms and support policies belong to Strawberry, not Orange.

## Features

- Local music playback and collection management, including playlists, queues, smart playlists and tag editing.
- GStreamer codec support for formats including FLAC, WAV, Ogg Vorbis, Opus, MP3, AAC and ALAC. Installed plugins determine which formats can play or be transcoded.
- ReplayGain, equalizer, audio analyzer, moodbar and waveform seek bar; optional EBU R128 analysis.
- Album cover and lyrics providers, with optional acoustic fingerprinting through Chromaprint.
- Internet radio: Radio Paradise, SomaFM, Radio Browser search and custom stream URLs.
- File browser with list and tree modes, configurable tree roots and access to mounted network shares.
- Subsonic-compatible servers and optional, unofficial Tidal, Spotify and Qobuz integrations.
- Scrobbling, desktop notifications, global shortcuts and Linux MPRIS2 control.
- Optional audio CD, USB/MTP/iPod device support and Discord Rich Presence.

Third-party services can require accounts, credentials or subscriptions and can change their APIs. Their presence in the interface does not guarantee current service availability. Platform and build options also affect feature availability.

## Using the updated GUI

- **Radio settings:** changes under Settings → Radio take effect when you Apply. A changed search cancels obsolete requests. Load more is disabled during a request and retries the same page after a connection failure.
- **Files:** type a folder path and press Enter to navigate. Enter on a selected file adds it using your configured activation behavior. In tree mode, removing a root removes the selected top-level tree entry, even when root paths overlap; it does not delete files.
- **Network shares:** mount the share in your file manager first. The network button lists supported existing mounts; Orange does not mount shares or collect share credentials.
- **Appearance:** Apply keeps the chosen appearance as the baseline for Cancel. The crop option requires both Stretch and Keep aspect ratio. Forced light/dark schemes require Qt 6.8 or newer; custom palette controls remain available with supported styles on earlier Qt versions.

## Build from source

Required: CMake 3.13+, a C++17 compiler, pkg-config, Boost, GLib, Qt 6.4+ (Core, Concurrent, Gui, Widgets, Network, SQL and applicable D-Bus support), SQLite 3.9+, GStreamer, TagLib 1.12+, ICU and KDSingleApplication 1.1.0+. Linux also requires ALSA development files.

Optional dependencies include Chromaprint, FFTW3, PulseAudio, libcdio, libmtp, libgpod and libebur128. Install GStreamer base and good plugins, plus bad, ugly or libav as needed for your codecs. CMake prints the enabled and disabled features during configuration.

```sh
git clone --recursive https://github.com/goshitsarch-eng/Orange.git
cd Orange
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
sudo cmake --install build
```

The executable is `orange`; the internal CMake target remains `strawberry` for upstream compatibility. Consult [.github/workflows/orange-ci.yml](.github/workflows/orange-ci.yml) for the Ubuntu 24.04 dependency list and a reproducible build/test sequence.

The inherited code contains Linux, BSD, macOS and Windows support. This maintenance pass does not certify every platform or third-party integration; see the audit for validation coverage.

## Tests

Install GTest, GMock and Qt Test before configuring. Test binaries are excluded from the default build and must be built explicitly:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_WERROR=ON
cmake --build build --target strawberry build_tests --parallel 2
QT_QPA_PLATFORM=offscreen ctest --test-dir build --output-on-failure --timeout 120
```

`gui_regressions_test` exercises appearance, file-tree activation/root removal and radio search behavior using controlled network replies. It needs no radio account or external service. The separate `lyrics_live_tests` target contacts real providers and is intentionally excluded from the offline suite.

Orange CI runs on pushes and pull requests. The inherited packaging workflow remains separate; building and testing a commit does not publish release binaries automatically.

## Credits and license

Orange is free software under GPL-3.0-or-later. See [COPYING](COPYING). Strawberry, Clementine and Amarok contributors retain their credits and copyright notices. Upstream Strawberry development can be supported through its [project page](https://www.strawberrymusicplayer.org/).
