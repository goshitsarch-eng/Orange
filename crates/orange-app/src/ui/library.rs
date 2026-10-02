use super::*;
use orange_core::song::Song;
use std::sync::Arc;
#[derive(Clone)]
struct LibraryView {
    songs: Arc<Vec<Song>>,
    busy: bool,
    has_directories: bool,
}
impl PartialEq for LibraryView {
    fn eq(&self, other: &Self) -> bool {
        Arc::ptr_eq(&self.songs, &other.songs)
            && self.busy == other.busy
            && self.has_directories == other.has_directories
    }
}

#[component]
pub(super) fn Library() -> Element {
    let mut c = use_context::<Context>();
    let view = use_memo(move || {
        let s = c.snapshot.read();
        LibraryView {
            songs: s.songs.clone(),
            busy: s.busy,
            has_directories: !s.directories.is_empty(),
        }
    });
    let snapshot = view();
    let mut genre = use_signal(String::new);
    let mut artist = use_signal(String::new);
    let mut album = use_signal(String::new);
    let filtered = orange_collection::filter::CollectionFilter::parse(&(c.search)());
    let base = match (c.smart)() {
        1 => crate::library::smart_highest_rated(&snapshot.songs),
        2 => crate::library::smart_recently_added(&snapshot.songs),
        3 => crate::library::smart_recently_played(&snapshot.songs),
        4 => crate::library::smart_never_played(&snapshot.songs),
        5 => crate::library::smart_most_played(&snapshot.songs),
        _ => snapshot.songs.as_ref().clone(),
    };
    let genres = base
        .iter()
        .map(|s| s.genre.clone())
        .filter(|s| !s.is_empty())
        .collect::<std::collections::BTreeSet<_>>();
    let artists = base
        .iter()
        .filter(|s| genre().is_empty() || s.genre == genre())
        .map(|s| s.display_artist().to_owned())
        .collect::<std::collections::BTreeSet<_>>();
    let albums = base
        .iter()
        .filter(|s| {
            (genre().is_empty() || s.genre == genre())
                && (artist().is_empty() || s.display_artist() == artist())
        })
        .map(|s| s.display_album().to_owned())
        .collect::<std::collections::BTreeSet<_>>();
    let songs = Arc::new(
        base.into_iter()
            .filter(|s| {
                filtered.matches(s)
                    && (genre().is_empty() || s.genre == genre())
                    && (artist().is_empty() || s.display_artist() == artist())
                    && (album().is_empty() || s.display_album() == album())
            })
            .collect::<Vec<_>>(),
    );
    let title = match (c.smart)() {
        1 => "My Top Rated",
        2 => "Recently Added",
        3 => "Recently Played",
        4 => "Never Played",
        5 => "Most Played",
        _ => "Music",
    };
    rsx! {
        section { class: "page",
            div { class: "page-heading",
                div {
                    h1 { {title} }
                    p { "Browse your collection. Music stays in its original location." }
                }
                button {
                    id: "add-folder",
                    class: "accent",
                    disabled: snapshot.busy,
                    onclick: move |_| super::chrome::native_action("folder", c),
                    "＋ Add Music Folder"
                }
            }
            div { class: "library-tools",
                input {
                    id: "search",
                    r#type: "search",
                    aria_label: "Search music",
                    placeholder: "Search title, artist, album…",
                    value: (c.search)(),
                    oninput: move |e| c.search.set(e.value()),
                }
                button {
                    id: "rescan",
                    disabled: snapshot.busy || !snapshot.has_directories,
                    onclick: move |_| c.dispatch.call(Action::Rescan),
                    "Rescan"
                }
                button {
                    id: "open-music",
                    onclick: move |_| super::chrome::native_action("open", c),
                    "Open Music…"
                }
            }
            div { class: "browser-filters",
                label {
                    "Genre"
                    select {
                        aria_label: "Filter by genre",
                        value: genre(),
                        onchange: move |e| {
                            genre.set(e.value());
                            artist.set(String::new());
                            album.set(String::new());
                        },
                        option { value: "", selected: genre().is_empty(), "All genres" }
                        for name in genres {
                            option {
                                value: name.clone(),
                                selected: genre() == name,
                                "{name}"
                            }
                        }
                    }
                }
                label {
                    "Artist"
                    select {
                        aria_label: "Filter by artist",
                        value: artist(),
                        onchange: move |e| {
                            artist.set(e.value());
                            album.set(String::new());
                        },
                        option { value: "", selected: artist().is_empty(), "All artists" }
                        for name in artists {
                            option {
                                value: name.clone(),
                                selected: artist() == name,
                                "{name}"
                            }
                        }
                    }
                }
                label {
                    "Album"
                    select {
                        aria_label: "Filter by album",
                        value: album(),
                        onchange: move |e| album.set(e.value()),
                        option { value: "", selected: album().is_empty(), "All albums" }
                        for name in albums {
                            option {
                                value: name.clone(),
                                selected: album() == name,
                                "{name}"
                            }
                        }
                    }
                }
            }
            if snapshot.songs.is_empty() {
                div { class: "empty",
                    super::chrome::Icon { name: "music" }
                    h2 { "Make room for your music" }
                    p { "Add a folder to browse albums, build playlists, and start listening." }
                }
            } else {
                TrackTable { songs }
            }
        }
    }
}

#[component]
pub(super) fn TrackTable(songs: Arc<Vec<Song>>) -> Element {
    let mut c = use_context::<Context>();
    let mut limit = use_signal(|| 200usize);
    let total = songs.len();
    rsx! {
        div { class: "tracks",
            table { aria_label: "Music tracks",
                thead {
                    tr {
                        th { "#" }
                        th { "Title" }
                        th { "Artist" }
                        th { "Album" }
                        th { "Length" }
                        th { class: "actions-column", "Actions" }
                    }
                }
                tbody {
                    for (index, song) in songs.iter().take(limit()).enumerate() {
                        {
                            let song = song.clone();
                            let chosen = song.clone();
                            let queued = song.clone();
                            let selected = song.clone();
                            let all = songs.clone();
                            let play = songs.clone();
                            let title = song.display_title();
                            rsx! {
                                tr {
                                    key: "{index}-{song.url}",
                                    tabindex: "0",
                                    class: if c.selected.read().as_ref().is_some_and(|s| s.url == song.url) { "selected" } else { "" },
                                    onclick: move |_| c.selected.set(Some(selected.clone())),
                                    ondoubleclick: move |_| c.dispatch.call(Action::PlayShared(all.clone(), index)),
                                    oncontextmenu: move |e| {
                                        e.prevent_default();
                                        c.selected.set(Some(chosen.clone()));
                                        c.dialog.set(Some(super::dialogs::Dialog::TrackActions));
                                    },
                                    onkeydown: move |e| {
                                        if e.key() == Key::Enter {
                                            c.dispatch.call(Action::PlayShared(play.clone(), index));
                                        }
                                    },
                                    td {
                                        if song.track > 0 {
                                            "{song.track}"
                                        }
                                    }
                                    td { class: "track-title", {title} }
                                    td { {song.display_artist()} }
                                    td { {song.display_album()} }
                                    td { {song.format_length()} }
                                    td { class: "row-actions",
                                        button {
                                            title: "Add to queue",
                                            aria_label: format!("Add {} to queue", song.display_title()),
                                            onclick: move |e| {
                                                e.stop_propagation();
                                                c.dispatch.call(Action::Enqueue(vec![queued.clone()]));
                                            },
                                            "＋"
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            div { class: "table-footer",
                span { "{total} tracks" }
                if total > limit() {
                    button { onclick: move |_| limit.set(limit() + 200), "Show more tracks" }
                }
            }
        }
    }
}
