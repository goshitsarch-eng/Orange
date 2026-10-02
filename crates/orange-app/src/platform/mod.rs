//! Native integrations are kept out of domain logic and UI components.
use dioxus::desktop::muda::{
    accelerator::{Accelerator, Code, Modifiers},
    Menu, MenuItem, PredefinedMenuItem, Submenu,
};
use std::path::{Path, PathBuf};

fn native_dialog() -> rfd::AsyncFileDialog {
    rfd::AsyncFileDialog::new().set_parent(&*dioxus::desktop::window().window)
}

pub async fn open_music() -> Option<Vec<PathBuf>> {
    native_dialog()
        .set_title("Open music")
        .add_filter(
            "Audio",
            &[
                "flac", "mp3", "wav", "ogg", "opus", "m4a", "aac", "aiff", "wma", "ape", "wv",
                "dsf", "dff",
            ],
        )
        .pick_files()
        .await
        .map(|files| files.into_iter().map(|f| f.path().to_owned()).collect())
}
pub async fn folder(title: &str) -> Option<PathBuf> {
    native_dialog()
        .set_title(title)
        .pick_folder()
        .await
        .map(|f| f.path().to_owned())
}
pub async fn playlist() -> Option<PathBuf> {
    native_dialog()
        .set_title("Import playlist")
        .add_filter("Playlists", &["m3u", "m3u8", "pls", "xspf"])
        .pick_file()
        .await
        .map(|f| f.path().to_owned())
}
pub async fn save(name: &str) -> Option<PathBuf> {
    native_dialog()
        .set_title("Choose a new output file")
        .set_file_name(name)
        .save_file()
        .await
        .map(|f| f.path().to_owned())
}
pub fn reveal(path: &Path) -> Result<(), String> {
    #[cfg(target_os = "windows")]
    {
        std::process::Command::new("explorer.exe")
            .arg({
                let mut argument = std::ffi::OsString::from("/select,");
                argument.push(path);
                argument
            })
            .spawn()
            .map_err(|e| e.to_string())?;
        Ok(())
    }
    #[cfg(target_os = "macos")]
    {
        std::process::Command::new("/usr/bin/open")
            .arg("-R")
            .arg(path)
            .spawn()
            .map_err(|e| e.to_string())?;
        Ok(())
    }
    #[cfg(not(any(target_os = "windows", target_os = "macos")))]
    {
        open::that_detached(path.parent().unwrap_or(path)).map_err(|e| e.to_string())
    }
}
pub fn open_project() -> Result<(), String> {
    open::that_detached("https://github.com/goshitsarch-eng/Orange").map_err(|e| e.to_string())
}
pub fn primary_modifier() -> Modifiers {
    #[cfg(target_os = "macos")]
    {
        Modifiers::SUPER
    }
    #[cfg(not(target_os = "macos"))]
    {
        Modifiers::CONTROL
    }
}
pub fn menu() -> Result<Menu, String> {
    let menu = Menu::new();
    let app = Submenu::new("Orange", true);
    let file = Submenu::new("File", true);
    let edit = Submenu::new("Edit", true);
    let playback = Submenu::new("Playback", true);
    let item = |id: &str, label: &str, key: Option<Code>| {
        MenuItem::with_id(
            id,
            label,
            true,
            key.map(|k| Accelerator::new(Some(primary_modifier()), k)),
        )
    };
    app.append_items(&[
        &item("about", "About Orange", None),
        &item("settings", "Settings…", Some(Code::Comma)),
        &PredefinedMenuItem::separator(),
        &item("quit", "Quit Orange", Some(Code::KeyQ)),
    ])
    .map_err(|e| e.to_string())?;
    #[cfg(target_os = "macos")]
    app.append_items(&[
        &PredefinedMenuItem::separator(),
        &PredefinedMenuItem::services(None),
        &PredefinedMenuItem::hide(None),
        &PredefinedMenuItem::hide_others(None),
        &PredefinedMenuItem::show_all(None),
    ])
    .map_err(|e| e.to_string())?;
    file.append_items(&[
        &item("open", "Open Music…", Some(Code::KeyO)),
        &item("folder", "Add Music Folder…", None),
        &item("import", "Import Playlist…", None),
        &item("export", "Export Queue…", None),
    ])
    .map_err(|e| e.to_string())?;
    edit.append_items(&[
        &PredefinedMenuItem::undo(None),
        &PredefinedMenuItem::redo(None),
        &PredefinedMenuItem::separator(),
        &PredefinedMenuItem::cut(None),
        &PredefinedMenuItem::copy(None),
        &PredefinedMenuItem::paste(None),
        &PredefinedMenuItem::select_all(None),
    ])
    .map_err(|e| e.to_string())?;
    playback
        .append_items(&[
            &item("play", "Play / Pause", None),
            &item("stop", "Stop", None),
            &item("next", "Next", None),
            &item("previous", "Previous", None),
            &item("stop-after", "Stop After Current", None),
            &PredefinedMenuItem::separator(),
            &item("undo", "Undo Queue Change", None),
            &item("redo", "Redo Queue Change", None),
        ])
        .map_err(|e| e.to_string())?;
    menu.append_items(&[&app, &file, &edit, &playback])
        .map_err(|e| e.to_string())?;
    Ok(menu)
}

/// Embedded identity asset; never depends on a desktop icon theme.
pub fn window_icon() -> Result<dioxus::desktop::tao::window::Icon, String> {
    let bytes = include_bytes!("../../../../data/icons/128x128/com.goshapps.Orange.png");
    let mut decoder = png::Decoder::new(std::io::Cursor::new(bytes));
    decoder.set_transformations(png::Transformations::EXPAND | png::Transformations::STRIP_16);
    let mut reader = decoder.read_info().map_err(|e| e.to_string())?;
    let mut buffer = vec![
        0;
        reader
            .output_buffer_size()
            .ok_or("Icon buffer unavailable")?
    ];
    let info = reader.next_frame(&mut buffer).map_err(|e| e.to_string())?;
    let rgba = match info.color_type {
        png::ColorType::Rgba => buffer[..info.buffer_size()].to_vec(),
        png::ColorType::Rgb => buffer[..info.buffer_size()]
            .as_chunks::<3>()
            .0
            .iter()
            .flat_map(|rgb| [rgb[0], rgb[1], rgb[2], 255])
            .collect(),
        _ => return Err("Icon must be RGB or RGBA".into()),
    };
    dioxus::desktop::tao::window::Icon::from_rgba(rgba, info.width, info.height)
        .map_err(|e| e.to_string())
}
