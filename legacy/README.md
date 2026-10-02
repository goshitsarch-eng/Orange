# Retained migration references

`cosmic-ui.rs` and `cosmic-icon-probe.rs` are the former Rust UI/test, retained
verbatim as reference material. `qt-build-workflow.yaml` is the previous Qt CI
configuration. The original `src/`, CMake, Qt tests, data/schema and historical
platform packaging remain in the repository.

The canonical Cargo binary uses Dioxus Desktop. These references are not enabled
by Cargo features or active CI, and no second runtime is shipped by the new
packaging recipes. Useful-feature parity with the complete Qt implementation has
not been established; see MIGRATION_AUDIT.md before deleting any legacy source.
The historical Qt build can still be investigated via its CMake tree; it was not
rebuilt or certified by this migration session.
