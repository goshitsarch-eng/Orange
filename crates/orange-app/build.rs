fn main() {
    #[cfg(target_os = "windows")]
    {
        println!("cargo:rerun-if-changed=../../dist/dioxus/windows/Orange.ico");
        let mut resource = winresource::WindowsResource::new();
        resource.set_icon("../../dist/dioxus/windows/Orange.ico");
        resource.set("ProductName", "Orange Music Player");
        resource.set("FileDescription", "Orange Music Player · Made by Gosh");
        resource.set(
            "LegalCopyright",
            "Gosh and Orange contributors; GPL-3.0-or-later",
        );
        if let Err(error) = resource.compile() {
            panic!("Cannot compile Windows resources: {error}");
        }
    }
}
