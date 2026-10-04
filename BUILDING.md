# Build and test Orange

The canonical desktop application is `desktop/`. Build on the target OS. Linux commands below were exercised in this cloud workspace; Windows/macOS/Flatpak commands are proposed automation and require target validation.

Use Flutter **3.47.6 stable**, its bundled Dart **3.13.5**, stable Rust (tested **1.99.0**, manifest minimum 1.89), Python 3.12, and flutter_rust_bridge_codegen **2.13.0**. `.fvmrc` pins Flutter; Cargo.lock and desktop/pubspec.lock pin dependencies. Flutter's official Git tag can bootstrap the SDK when the release manifest is unavailable:

```sh
git clone --depth 1 --branch 3.47.6 https://github.com/flutter/flutter.git /your/sdk/flutter
export PATH=/your/sdk/flutter/bin:$PATH
cargo install flutter_rust_bridge_codegen --version 2.13.0 --locked
cd desktop
flutter pub get --enforce-lockfile
```

On Debian/Ubuntu, native prerequisites are clang, cmake, ninja-build, pkg-config, libgtk-3-dev, liblzma-dev, libgles2, libssl-dev, libgstreamer1.0-dev and libgstreamer-plugins-base1.0-dev. Install GStreamer base/good/bad/ugly/libav plugins for the codec/conversion targets. Xvfb, xauth and a D-Bus session are needed for automated native UI QA. `file_selector` uses platform dialogs; install an appropriate xdg-desktop-portal backend for sandbox operation.

```sh
python3 scripts/desktop/build.py --debug
cd desktop
flutter run -d linux
```

Every Flutter Linux/Windows build compiles and installs its Rust products through CMake. macOS uses an Xcode phase. Debug builds use Cargo debug output; profile/release builds use Cargo release output. Native libraries must match the runner architecture.

From the repository root, independently test Rust:

```sh
cargo fmt --all -- --check
cargo test --workspace --no-default-features --locked
dbus-run-session -- cargo test --workspace --all-features --locked
cargo clippy --workspace --all-targets --all-features --locked -- -D warnings
cargo build -p orange-bridge --features native --locked
```

The full workspace command also tests the temporary migration reference and therefore needs WebKitGTK4.1, xdo and appindicator development packages. To test only the canonical backend, add `--exclude orange-app --exclude orange-theme` to workspace commands. On Windows/macOS omit `dbus-run-session`; MPRIS/udisks adapters are Linux-only.

```sh
cd desktop
flutter analyze
ORANGE_RUST_LIBRARY=/absolute/repository/target/debug/liborange_bridge.so dbus-run-session -- flutter test
cd ..
bash scripts/qa/run-flutter-linux.sh
python3 scripts/desktop/build.py --package
xvfb-run -a dbus-run-session -- python3 scripts/qa/smoke-linux-package.py target/packages/*.tar.gz
```

Use `.dll` on Windows and `.dylib` on macOS for bridge tests. `ORANGE_AUDIO_OUTPUT=null` explicitly selects silent QA output. It is never the normal playback default. `ORANGE_QA_SCREENSHOTS` optionally saves real Linux UI captures.

The cloud workspace has a non-root dependency installation. Source `/workspace/orange-env/activate.sh` before these commands. See [docs/cloud-environment.md](docs/cloud-environment.md). The resulting local Linux archive requires glibc 2.41 and host graphics drivers. CI's Ubuntu 24.04 baseline remains unverified.

## Windows (not yet executed)

Install Flutter Windows support, stable Rust MSVC, Visual Studio 2022 Desktop C++ tools, Windows SDK, Python 3.12, pkg-config and NSIS. `scripts/package/windows-sdk.ps1` downloads the GStreamer 1.26.5 runtime/development MSIs and verifies official SHA-256 sidecars before installation. For local use set `GSTREAMER_1_0_ROOT_MSVC_X86_64`, add its `bin` to PATH and set PKG_CONFIG_PATH to `lib/pkgconfig`. The script's GITHUB_ENV/GITHUB_PATH exports are intended for CI.

Run `python scripts/desktop/build.py --package`. Windows CMake installs orange_bridge.dll and orange-cli.exe into Flutter's Release bundle. Packaging copies that bundle, GStreamer DLLs/plugins/scanner and builds a per-user NSIS installer plus ZIP/checksums. No WebView2 installer or WebView runtime is part of the Flutter distribution. Execute installer/uninstaller, file associations, Unicode/long paths, audio, drag/drop and DPI QA before certification.

## macOS (not yet executed)

Install Xcode/command-line tools, CocoaPods as required by Flutter plugins, Flutter desktop, stable Rust, Python 3.12 and Homebrew `pkg-config gstreamer glib-networking`. Install Rust targets `aarch64-apple-darwin` and `x86_64-apple-darwin`.

Run `python3 scripts/desktop/build.py --package`. Xcode selects the native host architecture; the Rust build phase builds matching products. Separate Intel/Apple Silicon CI jobs produce matching apps, DMG, ZIP and checksums. The packaging script preserves the Flutter bundle, relocates non-system dylibs/plugins and verifies ad-hoc signing. Production signing/notarization needs a later credentialed release gate; local builds do not require a developer certificate. Finder open-event delivery, sandbox bookmark lifetime, native menus, Retina and physical audio require macOS QA.

## Flatpak (blocked in this environment)

The manifest targets GNOME 49 for Flutter's GTK host and GStreamer, plus Rust stable SDK extension 25.08. Prepare on the same architecture as the target:

```sh
python3 -m pip install 'aiohttp>=3.9.5,<4' 'tomlkit>=0.13.3,<1' 'PyYAML>=6.0.2,<7'
python3 scripts/flatpak/prepare_flutter.py
python3 scripts/flatpak/flatpak-cargo-generator.py Cargo.lock -o data/cargo-sources.json
python3 scripts/compat/check_manifest.py data/com.goshapps.Orange.yml
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user -y flathub org.gnome.Platform//49 org.gnome.Sdk//49 org.freedesktop.Sdk.Extension.rust-stable//25.08
flatpak-builder --user --install --force-clean --repo=flatpak-repo flatpak-build data/com.goshapps.Orange.yml
flatpak run com.goshapps.Orange
flatpak run --command=orange-cli com.goshapps.Orange --headless
flatpak build-bundle flatpak-repo orange.flatpak com.goshapps.Orange
```

SDK/Pub/Cargo downloads happen before offline compilation. The encoder gate intentionally fails a runtime missing advertised conversion capabilities. Dialogs request portal access, Music is read-only, and no blanket host/home filesystem permission is granted. Test install/start, portals, external-folder persistence, drag/drop, audio, MPRIS and both x86_64/aarch64 before release. Only manifest/source generation were checked locally; dl.flathub.org downloads currently return HTTP 403.
