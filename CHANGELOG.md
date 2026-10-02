# 3.1.0-alpha.1 — Rust + Dioxus migration preview

- Audit and retain existing COSMIC/Qt implementations before rewriting.
- Replace the canonical frontend with shared Dioxus Desktop components, native menus/dialogs, embedded controls and responsive themes.
- Add typed commands, a bounded latest-snapshot mailbox, background jobs and atomic settings with legacy appearance/volume import.
- Preserve statistics/ratings and song IDs during rescans; reject failed/cancelled scans without erasing the index.
- Wire playlist CRUD, import/export, queue undo/redo, repeat/shuffle, ratings, tag editing, conversion, radio/lyrics lookup and safe mounted-folder copying.
- Use native per-user paths and standard Unicode/Windows file URLs.
- Fix unsafe destination names, copy cancellation cleanup, XSPF XML parsing and equalizer playback interruption.
- Update Rustls and backport the upstream GLib VariantStrIter safety fix with an optimized regression.
- Add native OS/architecture CI, installer/bundle/Flatpak recipes and explicit QA/release gates. Windows/macOS/Flatpak support remains unverified.

# Changelog
