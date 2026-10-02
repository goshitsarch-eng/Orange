GLib 0.18.5 is retained because Dioxus Desktop's GTK 3 host and GStreamer
0.21 bindings require this ABI family. The registry archive was checked
against its original Cargo.lock SHA-256 before copying its source here.
The crate's original license and metadata are retained.

`glib/src/variant_iter.rs`: backport the upstream safety fix from
https://github.com/gtk-rs/gtk-rs-core/pull/1343 (RUSTSEC-2024-0429).
Make the output pointer mutable and pass `&mut p` to the variadic
`g_variant_get_child` FFI call. Passing `&p` permits optimized builds to
discard the C function's write and dereference a null pointer.

The safety fix changes two lines, no public API or native library ABI.
Mechanical lifetime annotations and redundant parentheses were also updated
using cargo fix for current Rust compiler diagnostics; those changes have no
behavioral effect. The
application has an optimized regression test for the affected iterator.
Remove this local patch when Dioxus Desktop can use GLib >= 0.20.
Version-only advisory scanners may still report this already-fixed function;
no advisory is suppressed in the project's audit configuration.
