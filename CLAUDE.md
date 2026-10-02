# Orange development

The canonical app is the Rust workspace and Dioxus Desktop, version
3.1.0-alpha.1. Read README.md, BUILDING.md, ARCHITECTURE.md,
MIGRATION_AUDIT.md and PLATFORM_SUPPORT.md before changing behavior.

Use locked Cargo builds. Run rustfmt, Clippy on all targets/features, domain
and full-feature tests; Linux full tests need a private D-Bus session for MPRIS.
The opt-in native WebView QA is documented in CONTRIBUTING.md. Native packages
must be tested on their actual OS; a Linux build proves no Windows/macOS result.

UI components render snapshots and send typed commands to the service owner.
Keep filesystem/network work outside rendering, preserve transactional data,
and use native OS paths/dialogs. Preserve user files, ratings and settings on
errors and cancellation. Do not advertise unimplemented account or hardware
capabilities. No telemetry or credential logging.

Historical Qt development instructions are retained in
legacy/qt-development.md. The Qt/COSMIC references remain until useful-feature
parity is demonstrated; the alpha is not a completed stable migration.
