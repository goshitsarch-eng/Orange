# Building Orange

Use stable Rust; the dependency minimum is 1.89. Local verification used 1.99.0.
The checked-in lockfile is authoritative. Build from the repository root with
`cargo build -p orange-app --locked`; use `--release` for packaging. The Dioxus
CLI is optional and is not required by these builds. No application Node runtime
or Qt toolkit is used by the canonical executable.

## Linux

On Debian/Ubuntu with WebKitGTK 4.1 available:

```sh
sudo apt-get update
sudo apt-get install pkg-config libgtk-3-dev libwebkit2gtk-4.1-dev libxdo-dev \
  libayatana-appindicator3-dev libssl-dev libgstreamer1.0-dev \
  libgstreamer-plugins-base1.0-dev gstreamer1.0-plugins-base \
  gstreamer1.0-plugins-good gstreamer1.0-plugins-bad gstreamer1.0-plugins-ugly \
  gstreamer1.0-libav xdg-desktop-portal xdg-desktop-portal-gtk
cargo build -p orange-app --release --locked
bash scripts/package/linux.sh
```

Choose a portal backend appropriate for the desktop (GTK, KDE or another
implementation providing FileChooser). The display/session bus must already be
available. The archive contains the binary, identity assets, desktop entry,
AppStream data, license and documentation. Install those assets into their usual
XDG locations if using the archive. Install the native runtime libraries on each
machine; the archive is not a self-contained AppImage. Builds on Debian 13 may
need newer libc than an Ubuntu 24.04 build; CI produces its archives on Ubuntu
24.04 for a defined baseline.

For native WebView regression tests install Xvfb and run
`bash scripts/qa/run-linux.sh`. This uses synthetic DOM events inside the actual
native WebView, an isolated profile, and GStreamer's synchronized null sink.
It does not verify physical speakers, desktop portals or Wayland. On Wayland,
launch normally in a real session and perform the QA checklist separately.

## Windows x86_64

Install stable Rust's `x86_64-pc-windows-msvc` toolchain and Visual Studio C++
build tools including the Windows SDK. Install both the GStreamer MSVC runtime
and development SDK, with all plugins. Do not mix MinGW and MSVC libraries.
WebView2 must be available to run the app (included with current Windows 11;
the generated installer also runs Microsoft's signed bootstrapper).

The CI SDK bootstrap script pins GStreamer 1.26.5 and verifies upstream SHA-256
sidecars before MSI installation. Its URLs and SDK layout are unverified here;
any upstream URL/checksum failure intentionally stops the build.

```powershell
$env:GSTREAMER_1_0_ROOT_MSVC_X86_64='C:\gstreamer\1.0\msvc_x86_64'
$env:PKG_CONFIG_PATH="$env:GSTREAMER_1_0_ROOT_MSVC_X86_64\lib\pkgconfig"
$env:PATH="$env:GSTREAMER_1_0_ROOT_MSVC_X86_64\bin;$env:PATH"
cargo build -p orange-app --release --locked
# Install NSIS, then build the archive and per-user installer:
./scripts/package/windows.ps1 -GStreamerRoot $env:GSTREAMER_1_0_ROOT_MSVC_X86_64
```

The installer creates Start Menu shortcuts, registers non-default Open With
handlers, installs a per-user uninstaller, and retains configuration/collection
data on uninstall. Test install, launch, associations and uninstall on a clean
Windows VM before publishing. `python scripts/qa/run-native.py` exercises an
isolated native WebView profile; it still requires a functioning desktop session.

## macOS Apple Silicon and Intel

Install Xcode command-line tools, stable Rust and Homebrew:

```sh
xcode-select --install
brew install pkg-config gstreamer
cargo build -p orange-app --release --locked
python3 scripts/package/macos.py
```

Build natively on each architecture; do not assume an x86_64 Homebrew prefix is
usable by an Apple Silicon build. Packaging creates `Orange.app` with bundle ID
`com.goshapps.Orange`, icon, document types and Retina metadata, copies GStreamer
plugins/helper and recursively relocates non-system dylibs. The script verifies
ad-hoc signatures and creates DMG/ZIP artifacts. No developer certificate is
required for local testing. Ad-hoc signing is not Apple notarization; public
Gatekeeper/signing behavior remains unverified. Perform Finder launch, Command
shortcuts, file dialogs, Retina and audio QA on each architecture.

## Flatpak x86_64 and aarch64

Install Flatpak/flatpak-builder, then:

```sh
flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.gnome.Platform//49 org.gnome.Sdk//49 \
  org.freedesktop.Sdk.Extension.rust-stable//25.08
python3 -m venv .venv-flatpak
.venv-flatpak/bin/pip install aiohttp tomlkit PyYAML
.venv-flatpak/bin/python scripts/flatpak/flatpak-cargo-generator.py Cargo.lock -o data/cargo-sources.json
flatpak-builder --user --install --force-clean --arch=x86_64 --repo=flatpak-repo \
  flatpak-build data/com.goshapps.Orange.yml
flatpak run com.goshapps.Orange
flatpak build-bundle flatpak-repo orange-3.1.0-alpha.1-x86_64.flatpak com.goshapps.Orange
```

Use a native aarch64 builder and change `--arch`/artifact name for ARM. GNOME 49
supplies the GTK/WebKit host; its Rust extension base is Freedesktop 25.08.
Cargo sources are pinned by original registry checksums and builds use offline,
locked Cargo. The local GLib patch travels as repository source.

Permissions cover display, GPU, audio, networking, music-folder access,
notifications and Orange's own MPRIS bus name. Shared IPC supports fallback X11;
Wayland is also enabled. Broad home/all-device/wildcard MPRIS grants were removed.
Other file access uses the OS document/file-chooser portal. Music-folder write
access supports explicitly requested tag edits. Verify actual portal grants,
WebKit subprocesses and audio inside the sandbox; a host launch is not proof.
Flathub access was denied by this cloud's proxy, so no sandbox build was completed.

## Verification and dependency audit

```sh
cargo fmt --all -- --check
cargo clippy --workspace --all-targets --all-features --locked -- -D warnings
cargo test --workspace --no-default-features --locked
# Linux: run full tests with dbus-run-session -- cargo test --workspace --all-features --locked
cargo test --workspace --all-features --locked
cargo test -p orange-media --release --features gst --test glib_variant --locked
cargo install cargo-audit --locked
cargo audit
python3 scripts/compat/check_manifest.py data/com.goshapps.Orange.yml
```

The audit currently has zero security vulnerabilities, with transitive
maintenance warnings and a build-only Rand warning; see PLATFORM_SUPPORT.md.
CI covers five native OS/architecture combinations and two Flatpak builds.
Workflow parsing does not establish that any remote job passes.
