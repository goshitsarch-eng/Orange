//! Native COSMIC window (`cosmic::Application` + `cosmic::widget`).
//! Nav: Collection / Playlists / Now Playing / Lyrics / Devices / Radio /
//! Settings, with a window-bound player bar, cosmic dialogs, and
//! Settings > Appearance = System / Light / Dark applied live via
//! `cosmic::command::set_theme` (System restores runtime follow, so portal
//! ColorScheme and cosmic-config switches apply with no restart).
//! Breeze icons are preferred when installed, COSMIC icons otherwise.

use cosmic::app::Settings;
use cosmic::iced::{Length, Size};
use cosmic::widget::nav_bar;
use cosmic::{executor, widget, Application, Core, Element};

use orange_core::appearance::AppearanceMode;
use orange_media::playback::Player;

use crate::about;
use crate::nav::Page;
use crate::playerbar::{fit_title, PlayerBarLayout};

#[cfg(feature = "dbus")]
use orange_media::mpris::BUS_NAME;
#[cfg(feature = "dbus")]
use orange_media::mpris_server::MprisCommand;

/// Messages for the Orange shell.
#[derive(Debug, Clone)]
pub enum Message {
    PlayPause,
    Next,
    Previous,
    VolumeChanged(f32),
    AppearanceSelected(AppearanceMode),
    SearchChanged(String),
    Noop,
}

pub struct OrangeApp {
    core: Core,
    nav: nav_bar::Model,
    player: Player,
    appearance: AppearanceMode,
    search: String,
    volume: f32,
    status: String,
    /// MPRIS remote state. The UI shell owns the queue; remotes drive it
    /// through the channel. Position stays 0 here (no audio clock in the
    /// window shell; `orange --serve` reports live positions).
    #[cfg(feature = "dbus")]
    mpris: Option<MprisShell>,
}

/// MPRIS plumbing for the window shell (feature `dbus`).
#[cfg(feature = "dbus")]
struct MprisShell {
    host: crate::mpris_host::MprisHost,
    commands: std::sync::mpsc::Receiver<MprisCommand>,
}

/// Launch the COSMIC window. Call after argument parsing.
pub fn run() -> Result<(), Box<dyn std::error::Error>> {
    prefer_breeze_icons();
    let settings = Settings::default().size(Size::new(1100.0, 700.0));
    cosmic::app::run::<OrangeApp>(settings, ())?;
    Ok(())
}

/// Prefer Breeze icons when installed, mirroring the 2.1.5 preference.
/// Falls back to COSMIC icons by leaving the default untouched.
fn prefer_breeze_icons() {
    if orange_theme::find_breeze_icon_dir(&orange_theme::system_icon_bases()).is_some() {
        cosmic::icon_theme::set_default("Breeze");
    }
}

impl OrangeApp {
    /// Drain MPRIS remote commands into the queue, publish the result back
    /// to the bus, and notify on track changes.
    #[cfg(feature = "dbus")]
    fn poll_remotes(&mut self) {
        // Disjoint field borrows in sequence so no `&self` call overlaps
        // the `&mut` borrow of the MPRIS shell.
        let commands: Vec<MprisCommand> = match self.mpris.as_mut() {
            Some(shell) => shell.commands.try_iter().collect(),
            None => return,
        };
        for command in &commands {
            // A remote Quit only stops playback in the window shell; the
            // window itself stays open (the daemon honors process exit).
            if crate::mpris_host::apply_command(&mut self.player, command) {
                self.status = String::from("Remote quit: playback stopped.");
            }
        }
        let title = self.now_playing_label();
        let changed = match self.mpris.as_mut() {
            Some(shell) => {
                let volume = self.player.volume() as f64 / 100.0;
                shell.host.sync_player(&self.player, 0, volume)
            }
            None => false,
        };
        if changed && self.player.current().is_some() {
            #[cfg(feature = "notify")]
            crate::notify::notify_track(&title, "", "");
        }
    }

    fn apply_appearance(&self) -> cosmic::app::Task<Message> {
        match self.appearance {
            AppearanceMode::Light => cosmic::command::set_theme(cosmic::Theme::light()),
            AppearanceMode::Dark => cosmic::command::set_theme(cosmic::Theme::dark()),
            // Restore runtime system-follow so portal/cosmic-config switches
            // apply live with no restart.
            AppearanceMode::System => {
                let system = self.core.system_theme().cosmic().clone();
                cosmic::command::set_theme(cosmic::Theme::system(std::sync::Arc::new(system)))
            }
        }
    }

    fn now_playing_label(&self) -> String {
        match self.player.current() {
            Some(track) if track.title.is_empty() => track.url.clone(),
            Some(track) => track.title.clone(),
            None => String::from("Not playing"),
        }
    }

    fn player_bar(&self) -> Element<'_, Message> {
        let playing = self.player.state() == orange_media::playback::EngineState::Playing;
        let play_icon = if playing {
            "media-playback-pause-symbolic"
        } else {
            "media-playback-start-symbolic"
        };
        // Window-bound: the title is fitted, never clipped past the controls.
        let title = fit_title(&self.now_playing_label(), 48);
        widget::row::with_capacity(6)
            .push(
                widget::button::icon(widget::icon::from_name("media-skip-backward-symbolic"))
                    .on_press(Message::Previous),
            )
            .push(
                widget::button::icon(widget::icon::from_name(play_icon))
                    .on_press(Message::PlayPause),
            )
            .push(
                widget::button::icon(widget::icon::from_name("media-skip-forward-symbolic"))
                    .on_press(Message::Next),
            )
            .push(widget::text(title).width(Length::Fill))
            .push(widget::text(format!("{}%", self.volume as u8)))
            .push(widget::slider(
                0.0..=100.0,
                self.volume,
                Message::VolumeChanged,
            ))
            .spacing(8)
            .padding(8)
            .into()
    }

    fn page_content(&self) -> Element<'_, Message> {
        let page = self
            .nav
            .active_data::<Page>()
            .copied()
            .unwrap_or(Page::Collection);
        match page {
            Page::Collection => widget::column::with_capacity(4)
                .push(
                    widget::text_input::search_input("Search collection", &self.search)
                        .on_input(Message::SearchChanged),
                )
                .push(widget::text(if self.search.is_empty() {
                    "Your collection appears here. Add music directories in Settings.".to_string()
                } else {
                    format!("Filtering for “{}” (live collection view).", self.search)
                }))
                .spacing(8)
                .into(),
            Page::Playlists => widget::column::with_capacity(4)
                .push(widget::text(format!(
                    "{} queued track(s).",
                    self.player.queue_len()
                )))
                .push(widget::text("Smart and dynamic playlists live here."))
                .spacing(8)
                .into(),
            Page::NowPlaying => {
                let layout = PlayerBarLayout { width: 600.0 };
                widget::column::with_capacity(4)
                    .push(widget::text(self.now_playing_label()))
                    .push(widget::text(format!(
                        "Analyzer {}",
                        if layout.analyzer_visible() {
                            "on"
                        } else {
                            "compact"
                        }
                    )))
                    .push(widget::text(&self.status))
                    .spacing(8)
                    .into()
            }
            Page::Lyrics => widget::column::with_capacity(4)
                .push(widget::text("Lyrics for the current track appear here."))
                .spacing(8)
                .into(),
            Page::Devices => widget::column::with_capacity(4)
                .push(widget::text(
                    "USB / MTP / iPod devices appear here for sync.",
                ))
                .spacing(8)
                .into(),
            Page::Radio => widget::column::with_capacity(4)
                .push(widget::text(
                    "Radio Paradise · SomaFM · Radio Browser · your streams.",
                ))
                .spacing(8)
                .into(),
            Page::Settings => {
                let mode_button = |mode: AppearanceMode, label: &'static str| {
                    widget::button::text(label).on_press(Message::AppearanceSelected(mode))
                };
                widget::scrollable(
                    widget::column::with_capacity(4)
                        .push(widget::text("Appearance"))
                        .push(
                            widget::row::with_capacity(3)
                                .push(mode_button(AppearanceMode::System, "System"))
                                .push(mode_button(AppearanceMode::Light, "Light"))
                                .push(mode_button(AppearanceMode::Dark, "Dark"))
                                .spacing(8),
                        )
                        .push(widget::text(format!(
                            "Active: {}",
                            self.appearance.as_str()
                        )))
                        .push(widget::text(about::title()))
                        .push(widget::text(about::maker_line()))
                        .push(widget::text(about::footer()))
                        .spacing(8)
                        .padding(16),
                )
                .into()
            }
        }
    }
}

impl Application for OrangeApp {
    type Executor = executor::Default;
    type Flags = ();
    type Message = Message;

    const APP_ID: &'static str = "com.goshapps.Orange";

    fn core(&self) -> &Core {
        &self.core
    }

    fn core_mut(&mut self) -> &mut Core {
        &mut self.core
    }

    fn init(core: Core, _flags: Self::Flags) -> (Self, cosmic::app::Task<Self::Message>) {
        let mut nav = nav_bar::Model::default();
        for page in Page::ALL {
            nav.insert().text(page.title()).data(*page);
        }
        nav.activate_position(0);
        #[cfg(feature = "dbus")]
        let mpris = {
            let host = crate::mpris_host::MprisHost::new();
            let (tx, commands) = std::sync::mpsc::channel();
            host.spawn_server(BUS_NAME.to_string(), tx);
            Some(MprisShell { host, commands })
        };
        (
            Self {
                core,
                nav,
                player: Player::new(),
                appearance: AppearanceMode::System,
                search: String::new(),
                volume: 100.0,
                status: String::from("Ready."),
                #[cfg(feature = "dbus")]
                mpris,
            },
            cosmic::app::Task::none(),
        )
    }

    fn nav_model(&self) -> Option<&nav_bar::Model> {
        Some(&self.nav)
    }

    fn on_nav_select(&mut self, id: nav_bar::Id) -> cosmic::app::Task<Self::Message> {
        self.nav.activate(id);
        cosmic::app::Task::none()
    }

    fn update(&mut self, message: Self::Message) -> cosmic::app::Task<Self::Message> {
        // MPRIS remotes are drained on every message so media keys stay
        // responsive even between ticks.
        #[cfg(feature = "dbus")]
        self.poll_remotes();
        match message {
            Message::PlayPause => {
                self.player.toggle_play_pause();
                self.status = format!("State: {:?}", self.player.state());
            }
            Message::Next => {
                self.status = String::from("Next track.");
            }
            Message::Previous => {
                self.status = String::from("Previous track.");
            }
            Message::VolumeChanged(volume) => {
                self.volume = volume.clamp(0.0, 100.0);
                self.player.set_volume(self.volume as u8);
            }
            Message::AppearanceSelected(mode) => {
                self.appearance = mode;
                return self.apply_appearance();
            }
            Message::SearchChanged(search) => {
                self.search = search;
            }
            Message::Noop => {}
        }
        cosmic::app::Task::none()
    }

    fn view(&self) -> Element<'_, Self::Message> {
        widget::column::with_capacity(4)
            .push(self.page_content())
            .push(self.player_bar())
            .into()
    }
}
