// SCRATCH QA probe (delete after diagnosis): resolve every icon name the UI
// uses through the exact libcosmic lookup the widgets use, and print results.
#![cfg(feature = "ui")]

const NAMES: &[&str] = &[
    "media-skip-backward-symbolic",
    "media-playback-start-symbolic",
    "media-playback-pause-symbolic",
    "media-skip-forward-symbolic",
    "media-playlist-repeat-symbolic",
    "media-playlist-shuffle-symbolic",
    "folder-music-symbolic",
    "audio-volume-high-symbolic",
    "open-menu-symbolic",
    "list-add-symbolic",
    "list-remove-symbolic",
    "media-playlist-consecutive-symbolic",
    "view-media-playlist-symbolic",
    "folder-saved-search-symbolic",
    "folder-symbolic",
    "document-text-symbolic",
    "drive-removable-media-symbolic",
    "radio-symbolic",
    "settings-symbolic",
];

#[test]
fn probe_icon_resolution() {
    for theme in ["Breeze", "breeze", "Adwaita", "hicolor"] {
        cosmic::icon_theme::set_default(theme);
        println!("--- default theme: {theme}");
        for name in NAMES {
            let hit = cosmic::widget::icon::from_name(*name).path();
            match hit {
                Some(p) => println!("OK   {name} -> {}", p.display()),
                None => println!("MISS {name}"),
            }
        }
    }
    cosmic::icon_theme::set_default("breeze");
    println!("--- candidate replacements (breeze, then Adwaita)");
    let candidates = [
        "preferences-system-symbolic",
        "settings-configure-symbolic",
        "configure-symbolic",
        "system-settings-symbolic",
        "document-edit-symbolic",
        "text-plain-symbolic",
        "accessories-text-editor-symbolic",
        "view-list-symbolic",
        "view-media-track-symbolic",
    ];
    for name in candidates {
        let hit = cosmic::widget::icon::from_name(name).path();
        match hit {
            Some(p) => println!("OK   {name} -> {}", p.display()),
            None => println!("MISS {name}"),
        }
    }
    cosmic::icon_theme::set_default("Adwaita");
    for name in candidates {
        let hit = cosmic::widget::icon::from_name(name).path();
        match hit {
            Some(p) => println!("OK   {name} -> {}", p.display()),
            None => println!("MISS {name}"),
        }
    }
    cosmic::icon_theme::set_default("Breeze");
    println!("--- Breeze with explicit size 16");
    for name in NAMES {
        let hit = cosmic::widget::icon::from_name(*name).size(16).path();
        match hit {
            Some(p) => println!("OK   {name} -> {}", p.display()),
            None => println!("MISS {name}"),
        }
    }
}
