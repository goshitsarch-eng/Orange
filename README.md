# :tangerine: Orange

Orange is a **music player and music collection organizer**, aimed at **audiophiles and music collectors**.
It is written in **C++ using GTK 4 and libadwaita**, with a GStreamer audio backend.

Orange is a fork of [Strawberry](https://www.strawberrymusicplayer.org), which was itself forked from
[Clementine](https://www.clementine-player.org) in 2018; Clementine began in 2010 as a port of
[Amarok](https://amarok.kde.org) 1.4. Most of the code here is Strawberry's work — see
[Credits and acknowledgements](#heart-credits-and-acknowledgements).

---

## :globe_with_meridians: Resources

- **Source and issues:** https://github.com/goshitsarch-eng/Orange

### Upstream

Orange tracks Strawberry, and Strawberry's own resources remain the reference for most of how the
application works:

- **Website:** https://www.strawberrymusicplayer.org
- **Wiki:** https://wiki.strawberrymusicplayer.org
- **Forum:** https://forum.strawberrymusicplayer.org
- **GitHub:** https://github.com/strawberrymusicplayer/strawberry
- **Translations:** https://crowdin.com/project/strawberrymusicplayer

---

## :warning: Opening an Issue

Report problems with Orange on the [Orange issue tracker](https://github.com/goshitsarch-eng/Orange/issues).

Before opening one:

1. **Check whether it also happens in Strawberry.** Orange inherits most of its behaviour, so a bug that
   reproduces there belongs [upstream](https://github.com/strawberrymusicplayer/strawberry/issues) and will
   reach more maintainers.
2. **Read the [Strawberry FAQ](https://wiki.strawberrymusicplayer.org/wiki/FAQ)** — most of it applies here.
3. **Search existing issues** to avoid duplicates.

---

## :moneybag: Supporting the upstream project

Orange is free software released under the GPL, and it exists because of Strawberry. If you find Orange
useful, please consider supporting **Strawberry's author**, Jonas Kvinge — these links fund Strawberry, not
Orange:

1. [Patreon](https://www.patreon.com/jonaskvinge)
2. [GitHub Sponsors](https://github.com/sponsors/jonaski)
3. [Ko-fi](https://ko-fi.com/jonaskvinge)
4. [PayPal](https://paypal.me/jonaskvinge)

---

## :white_check_mark: Features

- Play and organize your music collection
- Support for WAV, FLAC, Ogg FLAC, WavPack, Ogg Vorbis, Opus, Ogg Speex, MPC, TrueAudio, AIFF, MP4/AAC, ALAC, MP3, ASF, Monkey’s Audio, and DSD (DSF/DSDIFF)
- Bit-perfect playback on Linux
- MPRIS2 / D-Bus remote control on Linux
- Native desktop notifications
- Advanced playlist management
- Smart and dynamic playlists
- Audio analyzer, equalizer, moodbar, and waveform seek bar
- Volume normalization with ReplayGain and EBU R128 loudness analysis
- Editing tags, and fetching missing tags via acoustic fingerprinting using [AcoustID](https://acoustid.org/) and [MusicBrainz](https://musicbrainz.org/)
- Album cover art from: [Last.fm](https://www.last.fm/), [MusicBrainz](https://musicbrainz.org/), [Discogs](https://www.discogs.com/), [Musixmatch](https://www.musixmatch.com/), [Deezer](https://www.deezer.com/), [Tidal](https://www.tidal.com/), [Qobuz](https://www.qobuz.com/), [Spotify](https://www.spotify.com/)
- Lyrics from: [Genius](https://genius.com/), [Musixmatch](https://www.musixmatch.com/), [lyrics.ovh](https://lyrics.ovh/), [songlyrics](https://www.songlyrics.com/), [azlyrics](https://www.azlyrics.com/), [elyrics](https://www.elyrics.net/), [letras](https://www.letras.mus.br) and [lrclib.net](https://lrclib.net/)
- Audio format conversion (transcoding) to MP3, AAC, FLAC, Ogg Vorbis, Opus, Speex, WavPack, and ASF
- Music transfer to USB, MTP and iPod devices
- Scrobbling to [Last.fm](https://www.last.fm/), [ListenBrainz](https://listenbrainz.org/), and Subsonic
- Global keyboard shortcuts (Linux, macOS, and Windows)
- Discord Rich Presence
- Audio CD playback
- Internet radio from [Radio Paradise](https://radioparadise.com/), [SomaFM](https://somafm.com/), [Radio Browser](https://www.radio-browser.info/), and custom streams
- Streaming from Subsonic-compatible servers
- Unofficial Tidal, Spotify, and Qobuz integration

---

:white_check_mark: Tested on **Linux**, **OpenBSD**, **FreeBSD**, **macOS**, and **Windows**.

> **Note:** Orange is built from source. There are no prebuilt packages yet.

---

## :gear: Requirements

To build Orange from source, you’ll need:

**Dependencies:**
- [CMake 3.13 or higher](https://cmake.org/)
- C/C++ compiler ([GCC](https://gcc.gnu.org/), [Clang](https://clang.llvm.org/), or [MSVC](https://visualstudio.microsoft.com/vs/features/cplusplus/))
- [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/) or [pkgconf](https://github.com/pkgconf/pkgconf)
- [Boost](https://www.boost.org/)
- [GLib](https://developer.gnome.org/glib/)
- [GTK 4](https://www.gtk.org/)
- [libadwaita](https://gnome.pages.gitlab.gnome.org/libadwaita/)
- [libsoup 3](https://libsoup.gnome.org/)
- [json-glib](https://wiki.gnome.org/Projects/JsonGlib)
- [SQLite 3.9 or higher](https://www.sqlite.org)
- [ALSA (Linux only)](https://www.alsa-project.org/)
- [GStreamer](https://gstreamer.freedesktop.org/)
- [TagLib 1.12 or higher](https://www.taglib.org/)
- [ICU](https://unicode-org.github.io/icu/)

**Dependencies for optional features:**
- Fingerprinting & tagging: [Chromaprint](https://acoustid.org/chromaprint)
- Fast Spectrum Moodbar: [FFTW3](http://www.fftw.org/)
- PulseAudio integration: [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/)
- Audio CD support: [libcdio](https://www.gnu.org/software/libcdio/)
- MTP devices: [libmtp](http://libmtp.sourceforge.net/)
- iPod Classic: [libgpod](http://www.gtkpod.org/libgpod/)
- EBU R128 normalization: [libebur128](https://github.com/jiixyj/libebur128)

Also install GStreamer plugins **base**, **good**, and optionally **bad**, **ugly** and **libav** for full codec support.

---

## :wrench: Build from Source

**Get the code:**

    git clone --recursive https://github.com/goshitsarch-eng/Orange

**Build and install:**

    cd Orange
    cmake -S . -B build
    cmake --build build --parallel $(nproc)
    sudo cmake --install build

For building on Windows with Visual Studio 2022, Strawberry's toolchain applies unchanged:
:point_right: https://github.com/strawberrymusicplayer/strawberry-msvc-build-tools

---

## :heart: Credits and acknowledgements

### Lineage

Orange is the fourth link in a chain, and code and design from every earlier project survive here:

- **[Amarok](https://amarok.kde.org/)** — Mark Kretschmann, Max Howell and the Amarok team. Amarok 1.4 is the
  original ancestor of this codebase.
- **[Clementine](https://www.clementine-player.org/)** — David Sansome, John Maguire, Paweł Bara and
  Arnaud Bienner. Clementine began in 2010 as a port of Amarok 1.4 to Qt 4.
- **[Strawberry](https://www.strawberrymusicplayer.org/)** — Jonas Kvinge and the Strawberry contributors.
  Strawberry was forked from Clementine in 2018 and is the direct parent of Orange. The overwhelming majority
  of the code in this repository is theirs.

Full author and contributor lists for all four projects are shown in **Help → About Orange**.

### Built with

Orange would not exist without these free software projects:

| Component | Used for |
| --- | --- |
| [GTK 4](https://www.gtk.org/) and [libadwaita](https://gnome.pages.gitlab.gnome.org/libadwaita/) | User interface |
| [GLib / GObject / GIO](https://gitlab.gnome.org/GNOME/glib) | Core runtime, settings and I/O |
| [GStreamer](https://gstreamer.freedesktop.org/) | Audio decoding, playback and transcoding |
| [TagLib](https://taglib.org/) | Reading and writing audio tags |
| [SQLite](https://www.sqlite.org/) | Collection and playlist database |
| [libsoup](https://libsoup.gnome.org/) | HTTP |
| [json-glib](https://gitlab.gnome.org/GNOME/json-glib) | JSON parsing |
| [ICU](https://icu.unicode.org/) | Unicode collation and transliteration |
| [Boost](https://www.boost.org/) | Assorted C++ utilities |
| [libsecret](https://gitlab.gnome.org/GNOME/libsecret) | Storing credentials in the system keyring |
| [ALSA](https://www.alsa-project.org/) and [PulseAudio](https://www.freedesktop.org/wiki/Software/PulseAudio/) | Audio output on Linux |
| [Chromaprint / AcoustID](https://acoustid.org/chromaprint) and [MusicBrainz](https://musicbrainz.org/) | Acoustic fingerprinting and tag lookup |
| [libebur128](https://github.com/jiixyj/libebur128) | EBU R 128 loudness analysis |
| [FFTW](https://www.fftw.org/) | Fast spectrum moodbar |
| [libcdio](https://www.gnu.org/software/libcdio/) | Audio CD playback |
| [libmtp](https://libmtp.sourceforge.net/) and [libgpod](https://www.gtkpod.org/libgpod/) | MTP and iPod device support |

Cover art, lyrics and streaming metadata are provided by the services listed under
[Features](#white_check_mark-features); each remains the property of its respective provider.

### License

Orange is free software released under the **GNU General Public License, version 3 or later**, the same
licence it inherits from Strawberry. Copyright in the inherited code remains with Jonas Kvinge and the
Strawberry contributors. See [COPYING](COPYING) for the full text.
