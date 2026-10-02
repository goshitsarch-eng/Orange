use super::*;
use orange_core::song::Song;

#[component]
pub(super) fn Queue() -> Element {
    let mut c = use_context::<Context>();
    let snapshot = c.snapshot.read().clone();
    let mut limit = use_signal(|| 200usize);
    rsx! {
        section { class: "page",
            div { class: "page-heading",
                div {
                    h1 { "Play Queue" }
                    p { "{snapshot.queue.len()} tracks" }
                }
                div { class: "button-group",
                    button {
                        id: "save-playlist",
                        disabled: snapshot.queue.is_empty(),
                        onclick: move |_| c.dialog.set(Some(super::dialogs::Dialog::Playlist)),
                        "Save as Playlist"
                    }
                    button {
                        id: "clear-queue",
                        disabled: snapshot.queue.is_empty(),
                        onclick: move |_| c.dispatch.call(Action::ClearQueue),
                        "Clear"
                    }
                }
            }
            div { class: "button-group",
                label {
                    "Repeat"
                    select {
                        id: "repeat",
                        aria_label: "Repeat",
                        value: snapshot.settings.repeat.to_string(),
                        onchange: move |e| {
                            if let Ok(mode) = e.value().parse() {
                                c.dispatch.call(Action::Repeat(mode));
                            }
                        },
                        option {
                            value: "0",
                            selected: snapshot.settings.repeat == 0,
                            "Off"
                        }
                        option {
                            value: "1",
                            selected: snapshot.settings.repeat == 1,
                            "Playlist"
                        }
                        option {
                            value: "2",
                            selected: snapshot.settings.repeat == 2,
                            "Track"
                        }
                        option {
                            value: "3",
                            selected: snapshot.settings.repeat == 3,
                            "Album"
                        }
                    }
                }
                label {
                    "Shuffle"
                    select {
                        id: "shuffle",
                        aria_label: "Shuffle",
                        value: snapshot.settings.shuffle.to_string(),
                        onchange: move |e| {
                            if let Ok(mode) = e.value().parse() {
                                c.dispatch.call(Action::Shuffle(mode));
                            }
                        },
                        option {
                            value: "0",
                            selected: snapshot.settings.shuffle == 0,
                            "Off"
                        }
                        option {
                            value: "1",
                            selected: snapshot.settings.shuffle == 1,
                            "All tracks"
                        }
                        option {
                            value: "2",
                            selected: snapshot.settings.shuffle == 2,
                            "Within album"
                        }
                    }
                }
                button {
                    disabled: !snapshot.can_undo,
                    onclick: move |_| c.dispatch.call(Action::UndoQueue),
                    "Undo"
                }
                button {
                    disabled: !snapshot.can_redo,
                    onclick: move |_| c.dispatch.call(Action::RedoQueue),
                    "Redo"
                }
                button {
                    disabled: !snapshot.playback.is_active(),
                    onclick: move |_| c.dispatch.call(Action::StopAfterCurrent),
                    "Stop after current"
                }
            }
            if snapshot.queue.is_empty() {
                div { class: "empty",
                    h2 { "Your queue is empty" }
                    p { "Open music or add tracks from your library." }
                    button { onclick: move |_| super::chrome::native_action("open", c),
                        "Open Music…"
                    }
                }
            } else {
                div { class: "tracks",
                    table { aria_label: "Play queue",
                        thead {
                            tr {
                                th { "#" }
                                th { "Title" }
                                th { "Artist" }
                                th { "Album" }
                                th { "Length" }
                                th { "Actions" }
                            }
                        }
                        tbody {
                            for (index, track) in snapshot.queue.iter().take(limit()).enumerate() {
                                tr {
                                    key: "{index}",
                                    class: if snapshot.cursor == Some(index) { "selected" } else { "" },
                                    td { "{index+1}" }
                                    td {
                                        button {
                                            class: "text",
                                            onclick: move |_| c.dispatch.call(Action::PlayQueue(index)),
                                            "{track.title}"
                                        }
                                    }
                                    td { "{track.artist}" }
                                    td { "{track.album}" }
                                    td { {track.format_length()} }
                                    td {
                                        button {
                                            title: "Remove from queue",
                                            aria_label: format!("Remove {} from queue", track.title),
                                            onclick: move |_| c.dispatch.call(Action::RemoveQueue(index)),
                                            "Remove"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if snapshot.queue.len() > limit() {
                button { onclick: move |_| limit.set(limit() + 200), "Show more queued tracks" }
            }
        }
    }
}
#[component]
pub(super) fn Radio() -> Element {
    let c = use_context::<Context>();
    let mut name = use_signal(String::new);
    let mut url = use_signal(String::new);
    let mut query = use_signal(String::new);
    let snapshot = c.snapshot.read().clone();
    let stations = c.snapshot.read().settings.stations.clone();
    rsx! {
        section { class: "page",
            h1 { "Radio" }
            p { "Internet radio from Radio Paradise, SomaFM, and your saved stations." }
            div { class: "station-grid",
                for service in [orange_media::radio::radio_paradise(), orange_media::radio::somafm()] {
                    div { class: "card",
                        h2 { {service.name} }
                        for stream in service.streams {
                            {
                                let display = stream.name.clone();
                                rsx! {
                                    button {
                                        class: "station",
                                        onclick: move |_| {
                                            c
                                                .dispatch
                                                .call(
                                                    Action::Play(
                                                        vec![
                                                            Song {
                                                                title: stream.name.clone(),
                                                                artist: "Radio".into(),
                                                                url: stream.url.clone(),
                                                                ..Song::default()
                                                            },
                                                        ],
                                                        0,
                                                    ),
                                                )
                                        },
                                        "▶ {display}"
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if cfg!(feature = "online") {
                div { class: "form-row",
                    label {
                        "Search Radio Browser"
                        input {
                            aria_label: "Search radio stations",
                            value: query(),
                            oninput: move |e| query.set(e.value()),
                        }
                    }
                    button {
                        disabled: snapshot.busy,
                        onclick: move |_| {
                            #[cfg(feature = "online")] c.dispatch.call(Action::SearchRadio(query()));
                        },
                        "Search"
                    }
                }
            }
            for station in snapshot.radio_results {
                {
                    let display = station.name.clone();
                    rsx! {
                        div { class: "station-row",
                            strong { "{display}" }
                            button {
                                onclick: move |_| {
                                    c.dispatch.call(Action::AddStation(station.name.clone(), station.url.clone()))
                                },
                                "Save Station"
                            }
                        }
                    }
                }
            }
            h2 { "Your stations" }
            div { class: "form-row",
                label {
                    "Name"
                    input {
                        id: "station-name",
                        value: name(),
                        oninput: move |e| name.set(e.value()),
                    }
                }
                label {
                    "Stream URL"
                    input {
                        id: "station-url",
                        r#type: "url",
                        placeholder: "https://…",
                        value: url(),
                        oninput: move |e| url.set(e.value()),
                    }
                }
                button {
                    id: "save-station",
                    onclick: move |_| {
                        c.dispatch.call(Action::AddStation(name(), url()));
                    },
                    "Save Station"
                }
            }
            for (index, station) in stations.iter().enumerate() {
                {
                    let station = station.clone();
                    let display = station.name.clone();
                    rsx! {
                        div { class: "station-row",
                            button {
                                onclick: move |_| {
                                    c
                                        .dispatch
                                        .call(
                                            Action::Play(
                                                vec![
                                                    Song {
                                                        title: station.name.clone(),
                                                        artist: "Radio".into(),
                                                        url: station.url.clone(),
                                                        ..Song::default()
                                                    },
                                                ],
                                                0,
                                            ),
                                        )
                                },
                                "▶ {display}"
                            }
                            button {
                                aria_label: "Remove saved radio station",
                                onclick: move |_| c.dispatch.call(Action::RemoveStation(index)),
                                "Remove"
                            }
                        }
                    }
                }
            }
        }
    }
}
#[component]
pub(super) fn Files() -> Element {
    let c = use_context::<Context>();
    let mut limit = use_signal(|| 200usize);
    let s = c.snapshot.read().clone();
    use_hook(move || {
        if s.files_path.is_none() {
            let path = s
                .directories
                .first()
                .map(|d| std::path::PathBuf::from(&d.path))
                .unwrap_or_else(orange_core::paths::music_dir);
            c.dispatch.call(Action::BrowseFolder(path));
        }
    });
    let s = c.snapshot.read().clone();
    let parent = s
        .files_path
        .as_ref()
        .and_then(|path| path.parent())
        .map(std::path::Path::to_owned);
    let current = s.files_path.clone();
    rsx! {
        section { class: "page",
            h1 { "Files" }
            p {
                "Browse local music without adding it to your collection. You can also drop audio files onto the window."
            }
            div { class: "button-group",
                button {
                    id: "files-open",
                    class: "accent",
                    onclick: move |_| super::chrome::native_action("open", c),
                    "Open Music…"
                }
                button {
                    disabled: s.busy,
                    onclick: move |_| {
                        spawn(async move {
                            if let Some(path) = crate::platform::folder("Browse music folder").await {
                                c.dispatch.call(Action::BrowseFolder(path));
                            }
                        });
                    },
                    "Browse Folder…"
                }
                button {
                    disabled: s.busy,
                    onclick: move |_| {
                        spawn(async move {
                            if let Some(path) = crate::platform::folder("Play music folder").await {
                                c.dispatch.call(Action::PlayFolder(path));
                            }
                        });
                    },
                    "Play Folder…"
                }
            }
            div { class: "folder-row",
                button {
                    id: "files-up",
                    disabled: parent.is_none() || s.busy,
                    onclick: move |_| {
                        if let Some(path) = parent.clone() {
                            limit.set(200);
                            c.dispatch.call(Action::BrowseFolder(path));
                        }
                    },
                    "Up"
                }
                span { id: "files-path",
                    {
                        s.files_path
                            .as_ref()
                            .map(|p| p.display().to_string())
                            .unwrap_or_else(|| "Choose a folder".into())
                    }
                }
                button {
                    disabled: current.is_none() || s.busy,
                    onclick: move |_| {
                        if let Some(path) = current.clone() {
                            c.dispatch.call(Action::PlayFolder(path));
                        }
                    },
                    "Play This Folder"
                }
            }
            div { class: "tracks",
                table { aria_label: "Local music files",
                    tbody { id: "file-entries",
                        for (index, entry) in s.files.iter().take(limit()).enumerate() {
                            tr { key: "{index}",
                                td {
                                    button {
                                        class: "text",
                                        disabled: s.busy,
                                        onclick: {
                                            let path = entry.path.clone();
                                            let folder = entry.is_dir;
                                            move |_| {
                                                if folder {
                                                    limit.set(200);
                                                    c.dispatch.call(Action::BrowseFolder(path.clone()));
                                                } else {
                                                    c.dispatch.call(Action::OpenFiles(vec![path.clone()]));
                                                }
                                            }
                                        },
                                        if entry.is_dir {
                                            "▸ "
                                        }
                                        {entry.name.clone()}
                                    }
                                }
                                td {
                                    if entry.is_audio {
                                        button {
                                            onclick: {
                                                let path = entry.path.clone();
                                                move |_| c.dispatch.call(Action::OpenFiles(vec![path.clone()]))
                                            },
                                            "Play"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if s.files.len() > limit() {
                button { onclick: move |_| limit.set(limit() + 200), "Show more files" }
            }
            h2 { "Playlists" }
            div { class: "button-group",
                button { onclick: move |_| super::chrome::native_action("import", c), "Import Playlist…" }
                button { onclick: move |_| super::chrome::native_action("export", c), "Export Queue…" }
            }
        }
    }
}
#[component]
pub(super) fn Devices() -> Element {
    let c = use_context::<Context>();
    let s = c.snapshot.read().clone();
    rsx! {
        section { class: "page",
            h1 { "Devices" }
            p {
                "Copy the queue to a mounted music player or a folder. Tracks are organized by artist and album; existing files are preserved."
            }
            button {
                id: "sync-queue",
                class: "accent",
                disabled: s.queue.is_empty() || s.busy,
                onclick: move |_| {
                    spawn(async move {
                        if let Some(path) = crate::platform::folder("Copy queue to device or folder")
                            .await
                        {
                            c.dispatch.call(Action::Sync(path));
                        }
                    });
                },
                "Choose Destination…"
            }
            p { class: "note",
                "Use your operating system to make the device’s files available, then choose its music folder."
            }
        }
    }
}
#[component]
pub(super) fn SettingsPage() -> Element {
    let c = use_context::<Context>();
    let s = c.snapshot.read().clone();
    rsx! {
        section { class: "page settings",
            h1 { "Settings" }
            div { class: "card",
                h2 { "Appearance" }
                p { "Follow your system theme or choose a light or dark appearance." }
                label {
                    "Theme"
                    select {
                        id: "theme",
                        aria_label: "Theme",
                        value: s.settings.theme.clone(),
                        onchange: move |e| c.dispatch.call(Action::Theme(e.value())),
                        option {
                            value: "system",
                            selected: s.settings.theme == "system",
                            "Follow System"
                        }
                        option {
                            value: "light",
                            selected: s.settings.theme == "light",
                            "Light"
                        }
                        option {
                            value: "dark",
                            selected: s.settings.theme == "dark",
                            "Dark"
                        }
                    }
                }
            }
            div { class: "card",
                h2 { "Music folders" }
                for directory in s.directories {
                    div { class: "folder-row",
                        span { {directory.path} }
                        button {
                            disabled: s.busy,
                            onclick: move |_| c.dispatch.call(Action::RemoveFolder(directory.id)),
                            "Remove from Library"
                        }
                    }
                }
                button {
                    disabled: s.busy,
                    onclick: move |_| super::chrome::native_action("folder", c),
                    "Add Music Folder…"
                }
            }
            div { class: "card",
                h2 { "Equalizer" }
                p { "Ten-band equalizer. Gains update live without interrupting playback." }
                div { class: "eq-grid",
                    for (band, frequency) in orange_media::audio_fx::Equalizer::FREQUENCIES_HZ.iter().enumerate() {
                        label {
                            span { "{frequency} Hz" }
                            input {
                                id: format!("eq-{band}"),
                                r#type: "range",
                                aria_label: format!("Equalizer {frequency} Hz"),
                                min: "-12",
                                max: "12",
                                step: "0.5",
                                value: s.settings.equalizer[band].to_string(),
                                onchange: move |e| {
                                    if let Ok(gain) = e.value().parse() {
                                        c.dispatch.call(Action::Equalizer(band, gain));
                                    }
                                },
                            }
                            span { "{s.settings.equalizer[band]:.1} dB" }
                        }
                    }
                }
            }
            p { class: "note", "Made by Gosh · Orange {orange_core::version::VERSION}" }
        }
    }
}

#[component]
pub(super) fn Playlists() -> Element {
    let mut c = use_context::<Context>();
    let lists = c.snapshot.read().playlists.clone();
    rsx! {
        section { class: "page",
            h1 { "Playlists" }
            p { "Load a saved playlist into the queue, or save the current queue." }
            button { onclick: move |_| c.dialog.set(Some(super::dialogs::Dialog::Playlist)),
                "Save Queue as Playlist…"
            }
            if lists.is_empty() {
                p { "No saved playlists yet." }
            }
            for list in lists {
                div { class: "folder-row",
                    strong { "{list.name}" }
                    button {
                        onclick: move |_| {
                            c.dispatch.call(Action::LoadPlaylist(list.id));
                            c.page.set(Page::Queue);
                        },
                        "Load"
                    }
                    button { onclick: move |_| c.dispatch.call(Action::DeletePlaylist(list.id)),
                        "Delete"
                    }
                }
            }
        }
    }
}
