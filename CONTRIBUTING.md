# Contributing

The canonical executable is the Rust/Dioxus workspace. Read MIGRATION_AUDIT.md
and ARCHITECTURE.md before altering existing behavior. Legacy Qt/COSMIC sources
are reference material until parity is proven, not an alternative default app.

Keep domain logic out of components. Add typed actions in `commands.rs`, implement
behavior in the service/domain crates, and use the same actions from native menus,
buttons, keyboard commands and remote controls. Long file/network work belongs in
background jobs. Keep native paths as PathBuf, use standard file URLs, and isolate
OS APIs in platform modules. Do not construct shell command strings.

Preserve existing collections, statistics and configuration. Test cancellation
and error paths before changing destructive operations. Useful behavior must have
an explicit migration disposition; unverified backends must not become product
capability claims. Native UI and packaging need native platform evidence.

Use stable Rust and the lockfile. Required checks are formatting, Clippy with
warnings denied, portable/full tests, database compatibility and native WebView
QA as documented in BUILDING.md. Add focused regression tests for discovered
bugs; do not substitute self-agreement tests for real file/database operations.
Record platform limitations honestly and retain dependency licenses. Regenerate
the Flatpak source list after lockfile changes. Do not tag a stable release while
PLATFORM_SUPPORT.md lists unresolved release gates.
