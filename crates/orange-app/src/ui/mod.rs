//! Dioxus components render snapshots; they do not own application logic.
use crate::{commands::Action, nav::Page, service::Handle, state::Snapshot};
use dioxus::html::HasFileData;
use dioxus::prelude::*;
mod chrome;
mod dialogs;
mod library;
mod pages;
#[cfg(feature = "ui-qa")]
mod qa;

#[derive(Clone, Copy)]
struct Context {
    pub snapshot: Signal<Snapshot>,
    pub page: Signal<Page>,
    pub search: Signal<String>,
    pub smart: Signal<u8>,
    pub selected: Signal<Option<orange_core::song::Song>>,
    pub dialog: Signal<Option<dialogs::Dialog>>,
    pub dispatch: Callback<Action>,
}

pub fn run(uris: Vec<String>) -> Result<(), Box<dyn std::error::Error>> {
    use dioxus::desktop::{Config, LogicalSize, WindowBuilder};
    #[cfg(feature = "gst")]
    orange_media::backend_gst::prepare_bundled_runtime();
    let settings_path = crate::settings::Settings::path();
    let initial = crate::settings::Settings::load(&settings_path).unwrap_or_default();
    let handle = crate::service::start(
        orange_core::identity::collection_db_path(orange_core::paths::data_home()),
        settings_path,
        uris,
    )?;
    let events = handle.clone();
    let scale = std::sync::Arc::new(std::sync::atomic::AtomicU64::new(1.0f64.to_bits()));
    let initial_scale = scale.clone();
    let config = Config::new()
        .with_window(
            WindowBuilder::new()
                .with_title("Orange Music Player")
                .with_window_icon(Some(crate::platform::window_icon()?))
                .with_inner_size(LogicalSize::new(
                    initial.window_width,
                    initial.window_height,
                ))
                .with_min_inner_size(LogicalSize::new(600, 400)),
        )
        .with_data_directory(
            orange_core::identity::app_data_dir(orange_core::paths::data_home()).join("webview"),
        )
        .with_menu(crate::platform::menu()?)
        .with_on_window(move |window, _| {
            initial_scale.store(
                window.scale_factor().to_bits(),
                std::sync::atomic::Ordering::Relaxed,
            );
        })
        .with_custom_event_handler(move |event, _| {
            use dioxus::desktop::tao::event::{Event, WindowEvent};
            if let Event::WindowEvent { event, .. } = event {
                match event {
                    WindowEvent::ScaleFactorChanged { scale_factor, .. } => {
                        scale.store(scale_factor.to_bits(), std::sync::atomic::Ordering::Relaxed)
                    }
                    WindowEvent::Resized(size) => {
                        let _ = events.dispatch(Action::Window(
                            (f64::from(size.width)
                                / f64::from_bits(scale.load(std::sync::atomic::Ordering::Relaxed)))
                                as u32,
                            (f64::from(size.height)
                                / f64::from_bits(scale.load(std::sync::atomic::Ordering::Relaxed)))
                                as u32,
                        ));
                    }
                    WindowEvent::CloseRequested => {
                        if let Err(error) = events.shutdown(std::time::Duration::from_secs(2)) {
                            tracing::warn!("{error}");
                        }
                    }
                    _ => {}
                }
            }
        });
    dioxus::desktop::launch::launch(
        App,
        vec![Box::new(move || Box::new(handle.clone()))],
        vec![Box::new(config)],
    )
}

#[component]
fn App() -> Element {
    let handle = use_context::<Handle>();
    let mut snapshot = use_signal(Snapshot::default);
    let page = use_signal(|| Page::Library);
    let search = use_signal(String::new);
    let smart = use_signal(|| 0);
    let selected = use_signal(|| None);
    let dialog = use_signal(|| None);
    let commands = handle.clone();
    let dispatch = use_callback(move |action: Action| {
        if let Err(error) = commands.dispatch(action) {
            snapshot.write().error = Some(error);
        }
    });
    let context = Context {
        snapshot,
        page,
        search,
        smart,
        selected,
        dialog,
        dispatch,
    };
    use_context_provider(|| context);
    use_future(move || {
        let handle = handle.clone();
        async move {
            loop {
                if let Some(next) = handle.poll() {
                    if next.shutdown {
                        if let Err(error) = handle.shutdown(std::time::Duration::from_secs(2)) {
                            tracing::warn!("{error}");
                        }
                        dioxus::desktop::window().close();
                        return;
                    }
                    snapshot.set(next);
                }
                tokio::time::sleep(std::time::Duration::from_millis(80)).await;
            }
        }
    });
    let theme = use_memo(move || snapshot.read().settings.theme.clone());
    dioxus::desktop::use_muda_event_handler(move |event| {
        chrome::native_action(&event.id.0, context)
    });
    #[cfg(feature = "ui-qa")]
    qa::install(context);
    rsx! {
        style { {include_str!("theme.css")} }
        div { class:"app", "data-theme":theme(), onkeydown:move|event|chrome::shortcut(event,context),
            ondragover:move|event|event.prevent_default(),
            ondrop:move|event|{event.prevent_default();let paths=event.files().iter().map(|f|f.path()).collect::<Vec<_>>();if !paths.is_empty(){dispatch.call(Action::OpenFiles(paths));}},
            chrome::Header {}
            div { class:"workspace", chrome::Sidebar {} main { id:"main-content",match page(){
                Page::Library=>rsx!{library::Library {}},Page::Queue=>rsx!{pages::Queue {}},Page::Playlists=>rsx!{pages::Playlists {}},
                Page::Radio=>rsx!{pages::Radio {}},Page::Files=>rsx!{pages::Files {}},Page::Devices=>rsx!{pages::Devices {}},Page::Settings=>rsx!{pages::SettingsPage {}}
            }}}
            chrome::Status {}
            dialogs::Host {}
        }
    }
}
