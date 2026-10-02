#![cfg(feature = "gst")]

/// RUSTSEC-2024-0429 only reliably manifests with optimizations enabled.
/// CI runs this test in release mode as well as the ordinary full suite.
#[test]
fn variant_string_iteration_preserves_the_ffi_output_pointer() {
    use gstreamer::glib::variant::ToVariant;
    let values = ["Orange", "Música 日本", "last"];
    let variant = values.to_variant();
    assert_eq!(
        variant.array_iter_str().unwrap().collect::<Vec<_>>(),
        values
    );
    let mut iterator = variant.array_iter_str().unwrap();
    assert_eq!(iterator.nth(1), Some("Música 日本"));
    assert_eq!(iterator.next_back(), Some("last"));
}
