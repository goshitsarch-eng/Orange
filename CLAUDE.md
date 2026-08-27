# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Overview

Strawberry is a music player and music collection organizer, forked from Clementine in 2018. C++17, GTK 4 and libadwaita, GStreamer audio backend. Runs on Linux, *BSD, macOS, and Windows.

## Build & Test

```bash
# Configure + build (standard)
cmake -S . -B build
cmake --build build --parallel $(nproc)
sudo cmake --install build
```

Useful CMake options: `-DBUILD_WERROR=ON`, `-DENABLE_DEBUG_OUTPUT=ON`.

### Tests

Tests are built when GTest is found. They are `EXCLUDE_FROM_ALL`, so build them explicitly:

```bash
cmake --build build --target build_tests
cmake --build build --target run_strawberry_tests
./build/strawberry_tests
```

Test sources live in `tests/src/*_test.cpp`.

## Code Style

`clang-format` config is in `.clang-format` (LLVM-based, customized). Run it on changed files before committing. Notable: comments use long lines, one sentence per line, no 80-column wrapping.

Commit messages: first line is `ClassName: short explanation` (no trailing period); omit the class name if multiple classes change. Body (after a blank line) is prose explaining what/why. Reference issues with `Fixes #NNNN`. See `CONTRIBUTING.md`.

## Architecture

`src/main.cpp` constructs an `AdwApplication` and a single `Application` (`src/core/application.h`), which is the central dependency-injection container. Nearly every subsystem (database, player, collection, cover/lyrics providers, scrobbler, streaming/radio services, task manager, network) is created and owned here.

The UI is GTK 4 + libadwaita (`src/ui/`): `AdwApplicationWindow`, `AdwPreferencesDialog`, and `AdwAboutDialog`. Settings are stored with `GKeyFile`. HTTP uses libsoup 3. Persistence uses SQLite directly.

### Audio engine

`src/engine/gstengine.cpp` is the GStreamer backend. `Player` (`src/core/player.h`) drives playback, volume, and playlist navigation.

### Major subsystem directories (`src/`)

- `core/` — Application, Player, Database (SQLite), TaskManager, network, settings.
- `ui/` — Main window, settings dialog, tools dialogs.
- `collection/` — music library backend and directory scanning.
- `playlist/`, `queue/`, `playlistparsers/`, `smartplaylists/` — playlist management and persistence.
- `tagreader/` — tag reading/writing (TagLib).
- `covermanager/`, `lyrics/` — cover and lyrics providers.
- `streaming/`, `radios/` — Tidal, Qobuz, Spotify, Subsonic, and internet radio.
- `scrobbler/` — Last.fm / ListenBrainz / Subsonic scrobbling.
- `engine/` — GStreamer output and device finders.
- `device/` — USB/MTP/iPod/CD device support.
- `analyzer/`, `moodbar/`, `waveform/`, `equalizer/` — visualization and EQ.
- `mpris2/`, `globalshortcuts/`, `osd/`, `systemtrayicon/`, `discord/` — desktop integration.

### Optional features

Most features are compile-time optional, gated by `HAVE_*` macros generated into `config.h` (from `src/config.h.in`). Platform/feature code must be guarded with the matching `#ifdef HAVE_X` and CMake option.
