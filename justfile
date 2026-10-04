# Flutter desktop and independently testable Rust core.
run *args:
    cd desktop && flutter run -- {{args}}

build:
    python3 scripts/desktop/build.py

package:
    python3 scripts/desktop/build.py --package

bridge:
    RUST_LOG=info flutter_rust_bridge_codegen generate

test:
    cargo test --workspace --no-default-features --locked

test-full:
    cargo test --workspace --all-features --locked

headless:
    cargo run -p orange-cli --locked -- --headless

lint:
    cargo fmt --all -- --check
    cargo clippy --workspace --all-targets --all-features --locked -- -D warnings
    cd desktop && flutter analyze

qa-linux:
    bash scripts/qa/run-flutter-linux.sh

flatpak-vendor:
    python3 scripts/flatpak/prepare_flutter.py
    python3 scripts/flatpak/flatpak-cargo-generator.py Cargo.lock -o data/cargo-sources.json

flatpak-lint:
    python3 scripts/compat/check_manifest.py data/com.goshapps.Orange.yml
