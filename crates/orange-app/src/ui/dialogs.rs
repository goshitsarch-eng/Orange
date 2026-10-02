use super::*;
#[derive(Clone, PartialEq)]
pub(super) enum Dialog {
    About,
    Lyrics,
    Playlist,
    TrackActions,
    Tags,
    Transcode,
}
#[component]
pub(super) fn Host() -> Element {
    let mut c = use_context::<Context>();
    let dialog = (c.dialog)();
    use_effect(move || {
        let open = c.dialog.read().is_some();
        spawn(async move {
            let script = if open {
                r#"if(!document.activeElement?.closest('.dialog'))window.orangePreviousFocus=document.activeElement;
                document.querySelectorAll('.transport,.workspace,.status').forEach(n=>n.inert=true);
                const modal=document.querySelector('.dialog');
                if(modal){(modal.querySelector('[autofocus]')||modal.querySelector('button,input,select')||modal).focus();
                modal.onkeydown=e=>{if(e.key==='Tab'){const nodes=[...modal.querySelectorAll('button,input,select,textarea,[tabindex]')].filter(n=>!n.disabled&&n.tabIndex>=0);const first=nodes[0],last=nodes[nodes.length-1];if(e.shiftKey&&document.activeElement===first){e.preventDefault();last?.focus();}else if(!e.shiftKey&&document.activeElement===last){e.preventDefault();first?.focus();}}};}"#
            } else {
                r#"document.querySelectorAll('.transport,.workspace,.status').forEach(n=>n.inert=false);window.orangePreviousFocus?.focus();"#
            };
            if let Err(error) = document::eval(script).await {
                tracing::debug!("Dialog focus update failed: {error}");
            }
        });
    });
    let label = match dialog.as_ref() {
        Some(Dialog::About) => "About Orange",
        Some(Dialog::Lyrics) => "Lyrics",
        Some(Dialog::Playlist) => "Save playlist",
        Some(Dialog::TrackActions) => "Track actions",
        Some(Dialog::Tags) => "Edit tags",
        Some(Dialog::Transcode) => "Convert audio",
        None => "Orange dialog",
    };
    rsx! {if let Some(dialog)=dialog{div{class:"modal-backdrop",onclick:move|_|c.dialog.set(None),div{class:"dialog",tabindex:"-1",role:"dialog",aria_modal:"true",aria_label:label,onclick:move|e|e.stop_propagation(),
        button{class:"dialog-close",aria_label:"Close dialog",onclick:move|_|c.dialog.set(None),"×"}
        match dialog{Dialog::About=>rsx!{About{}},Dialog::Lyrics=>rsx!{Lyrics{}},Dialog::Playlist=>rsx!{Playlist{}},Dialog::TrackActions=>rsx!{TrackActions{}},Dialog::Tags=>rsx!{Tags{}},Dialog::Transcode=>rsx!{Transcode{}}}
    }}}}
}
#[component]
fn About() -> Element {
    let c = use_context::<Context>();
    rsx! {h1{"🍊 Orange Music Player"}p{"{orange_core::version::VERSION} · Made by Gosh"}p{"A music player and collection organizer. Rust and Dioxus Desktop. GPL-3.0-or-later. No telemetry."}p{class:"note",{orange_core::version::UPSTREAM_CREDIT}}button{onclick:move|_|{if let Err(e)=crate::platform::open_project(){c.dispatch.call(Action::ReportError(e));}},"Project Website"}}
}
#[component]
fn Playlist() -> Element {
    let mut c = use_context::<Context>();
    let mut name = use_signal(String::new);
    let lists = c.snapshot.read().playlists.clone();
    rsx! {h2{"Save queue as a playlist"}label{"Playlist name", input{id:"playlist-name",autofocus:true,value:name(),oninput:move|e|name.set(e.value())}}
        button{id:"playlist-save",disabled:name().trim().is_empty(),onclick:move|_|{c.dispatch.call(Action::SavePlaylist(name()));c.dialog.set(None);},"Save Playlist"}
        h3{"Saved playlists"}for list in lists{div{class:"folder-row",span{{list.name}}button{onclick:move|_|c.dispatch.call(Action::DeletePlaylist(list.id)),"Delete Playlist"}}}
    }
}
#[component]
fn Lyrics() -> Element {
    let c = use_context::<Context>();
    let s = c.snapshot.read().clone();
    let track = s.current();
    let stored = track
        .and_then(|t| s.songs.iter().find(|song| song.url == t.url))
        .map(|song| song.lyrics.clone())
        .filter(|l| !l.is_empty())
        .unwrap_or_else(|| "No stored lyrics for this track.".into());
    let lyrics = track
        .and_then(|track| {
            s.lyrics
                .as_ref()
                .filter(|(url, _)| *url == track.url)
                .map(|(_, text)| text.clone())
        })
        .unwrap_or(stored);
    rsx! {h2{"Lyrics"} if cfg!(feature="online"){button{disabled:s.busy||track.is_none(),onclick:move|_|{#[cfg(feature="online")]c.dispatch.call(Action::FetchLyrics);},"Look Up Lyrics (LRCLIB)"}}p{{track.map(|t|t.title.clone()).unwrap_or_else(||"Select a track".into())}}pre{class:"lyrics",{lyrics}}}
}
#[component]
fn TrackActions() -> Element {
    let mut c = use_context::<Context>();
    let song = (c.selected)();
    let local = song
        .as_ref()
        .and_then(|s| orange_core::paths::file_url_to_path(&s.url));
    let rating_url = song.as_ref().map(|s| s.url.clone()).unwrap_or_default();
    let rating = song.as_ref().map(|s| s.rating).unwrap_or(-1.0);
    rsx! {h2{"Track actions"}p{{song.as_ref().map(|s|s.display_title()).unwrap_or_default()}}
        label { "Rating", select { aria_label:"Track rating", value:rating.to_string(), onchange:move|e|if let Ok(value)=e.value().parse(){c.dispatch.call(Action::Rate(rating_url.clone(),value));}, option {value:"-1",selected:rating<0.0,"Unrated"} for stars in 1..=5{option{value:(f64::from(stars)/5.0).to_string(),selected:(rating-f64::from(stars)/5.0).abs()<0.0001,"{stars} stars"}} } }
        if let Some(song)=song{button{onclick:move|_|c.dispatch.call(Action::Enqueue(vec![song.clone()])),"Add to Queue"}}
        if let Some(path)=local{button{onclick:move|_|{if let Err(e)=crate::platform::reveal(&path){c.dispatch.call(Action::ReportError(e));}},"Show in Folder"}
            if cfg!(feature="tags"){button{onclick:move|_|c.dialog.set(Some(Dialog::Tags)),"Edit Tags…"}}
            if cfg!(feature="gst"){button{onclick:move|_|c.dialog.set(Some(Dialog::Transcode)),"Convert Audio…"}}
        }
    }
}
#[component]
fn Tags() -> Element {
    let mut c = use_context::<Context>();
    let song = (c.selected)().unwrap_or_default();
    let mut title = use_signal(|| song.title.clone());
    let mut artist = use_signal(|| song.artist.clone());
    let mut album = use_signal(|| song.album.clone());
    let mut genre = use_signal(|| song.genre.clone());
    let mut year = use_signal(|| {
        if song.year > 0 {
            song.year.to_string()
        } else {
            String::new()
        }
    });
    let mut track = use_signal(|| {
        if song.track > 0 {
            song.track.to_string()
        } else {
            String::new()
        }
    });
    let mut error = use_signal(String::new);
    rsx! {h2{"Edit Tags"}div{class:"tag-fields",label{"Title", input{value:title(),oninput:move|e|title.set(e.value())}}label{"Artist", input{value:artist(),oninput:move|e|artist.set(e.value())}}label{"Album", input{value:album(),oninput:move|e|album.set(e.value())}}label{"Genre", input{value:genre(),oninput:move|e|genre.set(e.value())}}label{"Year", input{value:year(),oninput:move|e|year.set(e.value())}}label{"Track", input{value:track(),oninput:move|e|track.set(e.value())}}}
        p{role:"alert",{error()}}
        button{onclick:move|_|{
            #[cfg(feature="tags")]if let Some(path)=orange_core::paths::file_url_to_path(&song.url){
                let parsed_year=if year().is_empty(){None}else{match year().parse::<u32>(){Ok(y)=>Some(y),Err(_)=>{error.set("Year must be a positive number".into());return;}}};
                let parsed_track=if track().is_empty(){None}else{match track().parse::<u32>(){Ok(t)if t>0=>Some(t),_=>{error.set("Track must be a positive number".into());return;}}};
                c.dispatch.call(Action::SaveTags(path,orange_media::tagger::TagPatch{title:Some(title()),artist:Some(artist()),album:Some(album()),genre:Some(genre()),year:parsed_year,track:parsed_track,disc:None}));c.dialog.set(None);
            }
        },"Save Tags"}
    }
}
#[component]
fn Transcode() -> Element {
    let mut c = use_context::<Context>();
    let song = (c.selected)().unwrap_or_default();
    let mut target = use_signal(|| "FLAC".to_string());
    rsx! {h2{"Convert Audio"}p{"Writes a new file. The original is never modified."}label{"Format", select{value:target(),onchange:move|e|target.set(e.value()),for preset in orange_core::codecs::TRANSCODE_TARGETS{option{value:preset.name,selected:target()==preset.name,{preset.name}}}}}
        button{onclick:move|_|{let value=target();let song=song.clone();spawn(async move{
            #[cfg(feature="gst")]if let (Some(source),Some(format))=(orange_core::paths::file_url_to_path(&song.url),orange_media::devices::transcode_target(&value)){if let Some(destination)=crate::platform::save(&format!("Converted.{}",format.extension)).await{c.dispatch.call(Action::Transcode(source,value,destination));c.dialog.set(None);}}
        });},"Choose Output File…"}
    }
}
