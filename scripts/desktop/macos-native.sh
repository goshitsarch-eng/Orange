#!/usr/bin/env bash
# Xcode build phase: produce exactly the architectures used by Flutter.
set -euo pipefail
repo_root="$(cd "$PROJECT_DIR/../.." && pwd)"
export PATH="${CARGO_HOME:-$HOME/.cargo}/bin:$PATH"
mode=debug
flags=()
if [[ "$CONFIGURATION" != Debug ]]; then mode=release; flags+=(--release); fi
libraries=()
helpers=()
for arch in $ARCHS; do
  case "$arch" in
    arm64) rust_target=aarch64-apple-darwin ;;
    x86_64) rust_target=x86_64-apple-darwin ;;
    *) echo "Unsupported macOS architecture: $arch" >&2; exit 1 ;;
  esac
  cargo build --manifest-path "$repo_root/Cargo.toml" -p orange-bridge -p orange-cli \
    --features native --locked --target "$rust_target" --target-dir "$repo_root/target" "${flags[@]}"
  libraries+=("$repo_root/target/$rust_target/$mode/liborange_bridge.dylib")
  helpers+=("$repo_root/target/$rust_target/$mode/orange-cli")
done
frameworks="$TARGET_BUILD_DIR/$FRAMEWORKS_FOLDER_PATH"
executables="$TARGET_BUILD_DIR/$EXECUTABLE_FOLDER_PATH"
mkdir -p "$frameworks" "$executables"
lipo -create "${libraries[@]}" -output "$frameworks/liborange_bridge.dylib"
lipo -create "${helpers[@]}" -output "$executables/orange-cli"
install_name_tool -id @rpath/liborange_bridge.dylib "$frameworks/liborange_bridge.dylib"
codesign --force --sign "${EXPANDED_CODE_SIGN_IDENTITY:--}" "$frameworks/liborange_bridge.dylib"
codesign --force --sign "${EXPANDED_CODE_SIGN_IDENTITY:--}" "$executables/orange-cli"
