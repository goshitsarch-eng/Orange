# Cloud development environment

The setup skill saved a reusable environment draft for goshitsarch-eng/Orange master. Saving a draft does not publish configuration or change the current network policy.

This Debian 13 workspace uses a non-root installation under `/workspace/orange-env`. Official signed APT packages are downloaded and extracted locally, with existing base-image headers/libraries reused through symlinks. Rust stable, Flutter 3.47.6/Dart 3.13.5, clang/cmake/ninja, GTK/GStreamer/plugins, Xvfb/portals, bridge generator 2.13.0 and cargo-audit 0.22.2 are available. The setup script preserves repository manifests/lockfiles and validates committed bindings without regeneration.

Start each shell with:

```sh
source /workspace/orange-env/activate.sh
cd /workspace/Orange
```

PUB_CACHE, XDG config/cache, analyzer state, Cargo/Rustup homes and native dependency paths use writable workspace directories. Supported Dart/Flutter analytics switches avoid attempts to write outside the workspace; HOME is unchanged. `ORANGE_PROFILE_DIR` isolates development settings/data from a normal user profile. Do not use this temporary profile for real user libraries.

The setup detects whether the reviewable Flutter migration patch is present: Flutter checkout validation builds the native bridge, debug desktop and real bridge/widget tests; the original master checkout instead builds/tests its audited Rust reference. The frontend source changes are independent of publishing this environment draft.

Package-manager network access is preserved. Explicit saved domains are storage.googleapis.com (official Flutter engine/Dart artifacts), pub.dev (Dart packages), api.github.com (CI inspection) and dl.flathub.org (Flatpak runtime). Direct requests to the last two were denied during setup; repository and PR access now works through the configured GitHub CLI. Flatpak runtime access remains unverified. No app-account secrets are required for core local development.

Use `bash scripts/qa/run-flutter-linux.sh` for native UI QA. It uses a disposable profile, Xvfb, D-Bus and an explicit null audio sink. Use `ORANGE_QA_SCREENSHOTS=/absolute/output` to capture native Linux windows. Release builds and package smoke tests are documented in BUILDING.md.

The retained WebKit reference has a helper-path overlay wrapper at `/workspace/orange-env/native-session.sh`; only its old QA needs a private user/mount namespace. Flutter does not need that wrapper or WebKit. Do not disable WebKit's security sandbox, create a network namespace, install unsigned packages or replace system libraries to run it.
