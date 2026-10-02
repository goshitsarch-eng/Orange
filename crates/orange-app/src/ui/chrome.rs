use super::*;
use orange_media::playback::EngineState;

#[component]
pub(super) fn Icon(name: String) -> Element {
    let path = match name.as_str() {
        "play" => "M8 5v14l11-7z",
        "pause" => "M6 5h4v14H6zm8 0h4v14h-4z",
        "next" => "M5 5v14l10-7zM17 5h2v14h-2z",
        "previous" => "M19 5v14L9 12zM5 5h2v14H5z",
        "stop" => "M6 6h12v12H6z",
        "music" => "M9 18V5l12-2v13h-2V6l-8 2v10zM9 18a3 3 0 1 1-3-3h3zm12-2a3 3 0 1 1-3-3h3z",
        _ => "M4 6h16v2H4zm0 5h16v2H4zm0 5h16v2H4z",
    };
    rsx! {svg{view_box:"0 0 24 24",width:"20",height:"20","aria-hidden":"true",path{d:path}}}
}
#[component]
pub(super) fn Header() -> Element {
    let mut c = use_context::<Context>();
    let s = c.snapshot.read().clone();
    let playing = s.playback == EngineState::Playing;
    let current = s.current();
    let title = current
        .map(|t| t.title.clone())
        .unwrap_or_else(|| "Not playing".into());
    let subtitle = current
        .map(|t| format!("{} · {}", t.artist, t.album))
        .unwrap_or_else(|| "Your music, your collection".into());
    let position = orange_core::song::format_duration_secs(s.position as i64);
    let duration = orange_core::song::format_duration_secs(s.duration as i64);
    rsx! {header{class:"transport",
        div{class:"brand",title:"Orange Music Player","🍊"}
        div{class:"transport-buttons",
            button{id:"previous",class:"icon",title:"Previous track",aria_label:"Previous track",disabled:s.queue.is_empty(),onclick:move|_|c.dispatch.call(Action::Previous),Icon{name:"previous"}}
            button{id:"play-pause",class:"icon primary",title:if playing{"Pause"}else{"Play"},aria_label:if playing{"Pause"}else{"Play"},disabled:s.queue.is_empty(),onclick:move|_|c.dispatch.call(Action::PlayPause),Icon{name:if playing{"pause"}else{"play"}}}
            button{id:"next",class:"icon",title:"Next track",aria_label:"Next track",disabled:s.queue.is_empty(),onclick:move|_|c.dispatch.call(Action::Next),Icon{name:"next"}}
            button{id:"stop",class:"icon",title:"Stop",aria_label:"Stop",disabled:!s.playback.is_active(),onclick:move|_|c.dispatch.call(Action::Stop),Icon{name:"stop"}}
        }
        div{class:"now-playing",strong{id:"now-title",{title}}span{{subtitle}} div{class:"spectrum",role:"img",aria_label:"Audio spectrum",for gain in s.spectrum.iter(){span{style:format!("height:{}%;",((gain+80.0)/80.0*100.0).clamp(0.0,100.0))}}}}
        div{class:"seek",span{{position}}input{id:"seek",r#type:"range",aria_label:"Playback position",title:"Seek",min:"0",max:s.duration.max(1).to_string(),value:s.position.to_string(),disabled:s.duration==0||!s.playback.is_active(),onchange:move|e|{if let Ok(seconds)=e.value().parse(){c.dispatch.call(Action::Seek(seconds));}}}span{{duration}}}
        label{class:"volume",title:"Volume","Volume", input{id:"volume",r#type:"range",aria_label:"Volume",min:"0",max:"100",value:s.settings.volume.to_string(),oninput:move|e|{if let Ok(value)=e.value().parse(){c.dispatch.call(Action::Volume(value));}}}}
        details{class:"app-menu",summary{aria_label:"Application menu",title:"Application menu","☰"}div{class:"menu-items",
            button{onclick:move|_|native_action("open",c),"Open Music…"}
            button{onclick:move|_|native_action("folder",c),"Add Music Folder…"}
            button{onclick:move|_|native_action("import",c),"Import Playlist…"}
            button{onclick:move|_|native_action("export",c),"Export Queue…"}
            button{onclick:move|_|c.page.set(Page::Settings),"Settings"}
            button{onclick:move|_|c.dialog.set(Some(super::dialogs::Dialog::Lyrics)),"Lyrics"}
            button{onclick:move|_|c.dialog.set(Some(super::dialogs::Dialog::About)),"About Orange"}
        }}
    }}
}
#[component]
pub(super) fn Sidebar() -> Element {
    let mut c = use_context::<Context>();
    let playlists = c.snapshot.read().playlists.clone();
    rsx! {nav{class:"sidebar",aria_label:"Music sources",
        div{class:"section-label","LIBRARY"}
        for page in Page::ALL.iter().copied(){button{id:format!("nav-{}",page.icon_key()),class:if (c.page)()==page{"source active"}else{"source"},onclick:move|_|{c.page.set(page);c.smart.set(0);c.search.set(String::new());},Icon{name:if page==Page::Library{"music"}else{"list"}}{page.title()}}}
        div{class:"section-label","SMART PLAYLISTS"}
        for (label,mode) in [("My Top Rated",1),("Recently Added",2),("Recently Played",3),("Never Played",4),("Most Played",5)]{button{class:"source",onclick:move|_|{c.page.set(Page::Library);c.smart.set(mode);},"{label}"}}
        div{class:"section-label","SAVED PLAYLISTS"}
        for list in playlists{button{class:"source",onclick:move|_|{c.dispatch.call(Action::LoadPlaylist(list.id));c.page.set(Page::Queue);},"{list.name}"}}
        button{id:"new-playlist",class:"source",onclick:move|_|c.dialog.set(Some(super::dialogs::Dialog::Playlist)),"＋ Save queue as playlist"}
    }}
}
#[component]
pub(super) fn Status() -> Element {
    let c = use_context::<Context>();
    let s = c.snapshot.read();
    let status = s.status.clone();
    let songs = s.songs.len();
    let can_cancel = s.busy && s.can_cancel;
    let error = s.error.clone();
    rsx! {footer{class:"status",span{"{songs} songs"}span{role:"status",{status}}if can_cancel{button{id:"cancel-operation",onclick:move|_|c.dispatch.call(Action::Cancel),"Cancel"}}}
        if let Some(error)=error{div{class:"error",role:"alert",span{{error}}button{aria_label:"Dismiss error",onclick:move|_|c.dispatch.call(Action::DismissError),"Dismiss"}}}
    }
}
pub(super) fn native_action(id: &str, mut c: Context) {
    match id {
        "open" => {
            spawn(async move {
                if let Some(paths) = crate::platform::open_music().await {
                    c.dispatch.call(Action::OpenFiles(paths));
                }
            });
        }
        "folder" => {
            spawn(async move {
                if let Some(path) = crate::platform::folder("Add music folder").await {
                    c.dispatch.call(Action::AddFolder(path));
                }
            });
        }
        "import" => {
            spawn(async move {
                if let Some(path) = crate::platform::playlist().await {
                    c.dispatch.call(Action::ImportPlaylist(path));
                    c.page.set(Page::Queue);
                }
            });
        }
        "export" => {
            spawn(async move {
                if let Some(path) = crate::platform::save("Orange playlist.m3u8").await {
                    c.dispatch.call(Action::ExportPlaylist(path));
                }
            });
        }
        "settings" => c.page.set(Page::Settings),
        "about" => c.dialog.set(Some(super::dialogs::Dialog::About)),
        "play" => c.dispatch.call(Action::PlayPause),
        "stop" => c.dispatch.call(Action::Stop),
        "next" => c.dispatch.call(Action::Next),
        "previous" => c.dispatch.call(Action::Previous),
        "stop-after" => c.dispatch.call(Action::StopAfterCurrent),
        "undo" => c.dispatch.call(Action::UndoQueue),
        "redo" => c.dispatch.call(Action::RedoQueue),
        "quit" => {
            if let Err(error) = dioxus::prelude::consume_context::<Handle>()
                .shutdown(std::time::Duration::from_secs(2))
            {
                tracing::warn!("{error}");
            }
            dioxus::desktop::window().close();
        }
        _ => {}
    }
}
pub(super) fn shortcut(event: KeyboardEvent, mut c: Context) {
    let editable = matches!(event.key(), Key::Character(_)) && event.modifiers().is_empty();
    if event.modifiers().contains(Modifiers::CONTROL) || event.modifiers().contains(Modifiers::META)
    {
        if let Key::Character(key) = event.key() {
            match key.to_lowercase().as_str() {
                "o" => {
                    event.prevent_default();
                    native_action("open", c)
                }
                "f" => {
                    event.prevent_default();
                    spawn(async move {
                        let _ = document::eval("document.getElementById('search')?.focus()").await;
                    });
                }
                "q" => native_action("quit", c),
                "," => c.page.set(Page::Settings),
                _ => {}
            }
        }
    } else if event.key() == Key::Escape {
        c.dialog.set(None);
    } else if event.key() == Key::F5 {
        c.dispatch.call(Action::Rescan);
    } else if editable { /* Text input keeps its native editing behavior. */
    }
}
