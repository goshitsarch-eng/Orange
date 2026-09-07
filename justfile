# Orange 3.0.0 (Rust + libcosmic) developer recipes.
# The legacy C++/Qt 2.1.5 build keeps working via `cmake -S . -B build`.

# Fast local check: builds + tests everything without Qt, libcosmic, or GStreamer.
test:
    cargo test --workspace --offline

# Full-featured check (needs network on first run for the libcosmic git checkout,
# system GStreamer dev files for `gst`): everything the Flatpak ships.
test-full:
    cargo test --workspace --features orange-app/full

# Headless launch: opens the existing collection read-only, prints a summary.
# No Qt required. Strawberry data is never touched.
run *args:
    cargo run --offline -p orange-app -- {{args}}

# COSMIC window (needs a Wayland/X11 session; same binary, UI enabled).
run-ui *args:
    cargo run --offline -p orange-app --features orange-app/ui -- {{args}}

lint:
    cargo clippy --workspace --all-targets --offline -- -D warnings
    cargo fmt --all -- --check

# Cross-oracle DB compat check: writes a 2.1.5-shaped fixture DB with CPython's
# sqlite3 (independent of rusqlite), then the Rust suite opens it read-only.
test-compat:
    python3 scripts/compat/make_fixture_db.py /tmp/orange-compat-fixture.db
    ORANGE_COMPAT_FIXTURE=/tmp/orange-compat-fixture.db cargo test --offline -p orange-db -- --nocapture

# Flatpak bundles for both arches. Single manifest, no hardcoded arch:
# pass --arch explicitly (defaults to host arch when omitted).

# Regenerate the pinned cargo sources after any Cargo.lock change.
# Needs `pip install aiohttp tomlkit PyYAML` once.
flatpak-vendor:
    python3 scripts/flatpak/flatpak-cargo-generator.py Cargo.lock -o data/cargo-sources.json

flatpak-x64: flatpak-vendor
    flatpak-builder --force-clean --arch=x86_64 --repo=repo-x64 build-x64 data/com.goshapps.Orange.yml
    flatpak build-bundle repo-x64 orange-3.0.0-x86_64.flatpak com.goshapps.Orange

flatpak-arm64: flatpak-vendor
    flatpak-builder --force-clean --arch=aarch64 --repo=repo-arm64 build-arm64 data/com.goshapps.Orange.yml
    flatpak build-bundle repo-arm64 orange-3.0.0-aarch64.flatpak com.goshapps.Orange

# Validate the manifest without building (no flatpak-builder required).
flatpak-lint:
    python3 scripts/compat/check_manifest.py data/com.goshapps.Orange.yml
