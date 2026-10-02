# Rust + Dioxus Desktop recipes. Native dependencies: BUILDING.md.
test:
    cargo test --workspace --no-default-features --locked

test-full:
    cargo test --workspace --all-features --locked

run *args:
    cargo run -p orange-app --locked -- {{args}}

headless:
    cargo run -p orange-app --locked -- --headless

lint:
    cargo fmt --all -- --check
    cargo clippy --workspace --all-targets --all-features --locked -- -D warnings

qa-linux:
    bash scripts/qa/run-linux.sh

flatpak-vendor:
    python3 scripts/flatpak/flatpak-cargo-generator.py Cargo.lock -o data/cargo-sources.json

flatpak-lint:
    python3 scripts/compat/check_manifest.py data/com.goshapps.Orange.yml

package-linux:
    cargo build -p orange-app --release --locked
    bash scripts/package/linux.sh
