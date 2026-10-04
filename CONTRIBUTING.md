# Contributing

The canonical application is desktop/ (Flutter) with UI-independent Rust services and domain crates. Read ARCHITECTURE.md and MIGRATION_AUDIT.md before changing ownership or removing reference behavior. Follow BUILDING.md for dependencies, binding regeneration, meaningful Rust/Dart/native UI checks and packaging.

Keep widgets/presentation state in Dart and domain/persistence/media rules in Rust. Use typed coarse bridge commands and update generated Rust/Dart bindings together. Never hand-edit generated bindings or add a second frontend to release bundles. Test Unicode paths, cancellation, errors and persistence with the real Rust bridge when changing those boundaries.

Changes affecting native plugins, file grants, architecture, media runtime or packaging require actual OS/sandbox QA. Record missing evidence rather than marking targets supported from compilation alone. Do not publish or tag a complete cross-platform release while the inventory still has required parity/platform gates.
