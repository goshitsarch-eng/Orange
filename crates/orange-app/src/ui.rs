//! Native COSMIC window: Strawberry-style source sidebar + playlist table +
//! context pane + player bar. Collection folders can be added, scanned, and
//! browsed as Album artist → Album → track, then queued and played.

use std::path::PathBuf;
use std::time::Duration;

use cosmic::app::Settings;
use cosmic::iced::{Alignment, Length, Size};
use cosmic::widget::nav_bar;
use cosmic::{executor, widget, Application, Core, Element};

use orange_collection::tree::GroupBy;
use orange_core::appearance::AppearanceMode;
use orange_core::song::Song;
use orange_media::audio_fx::Equalizer;
use orange_media::devices::DeviceFamily;
use orange_media::playback::{EngineState, Player, QueuedTrack};
use orange_media::radio::{radio_paradise, somafm, validate_custom_stream, CustomStream};
use orange_playlist::model::{RepeatMode, ShuffleMode};

use crate::about;
use crate::files::FileBrowser;
use crate::library::{smart_highest_rated, smart_most_played, smart_never_played, CollectionState};
use crate::nav::Page;
use crate::playerbar::{analyzer_bars, fit_title, seek_ratio, PlayerBarLayout};

#[cfg(feature = "gst")]
use orange_media::backend::{AudioSink, FxChain, PlaybackChain};
#[cfg(feature = "gst")]
use orange_media::backend_gst::{EngineEvent, GstEngine};
#[cfg(feature = "dbus")]
use orange_media::mpris::BUS_NAME;
#[cfg(feature = "dbus")]
use orange_media::mpris_server::MprisCommand;

/// Messages for the Orange shell.
#[derive(Debug, Clone)]
pub enum Message {
    PlayPause,
    Stop,
    Next,
    Previous,
    Seek(f32),
    VolumeChanged(f32),
    AppearanceSelected(AppearanceMode),
    SearchChanged(String),
    AddPathChanged(String),
    AddFolder,
    RemoveFolder(i64),
    Rescan,
    ToggleArtist(String),
    ToggleAlbum { artist: String, album: String },
    PlaySong(String),
    EnqueueSong(String),
    PlayArtist(String),
    EnqueueArtist(String),
    PlayAlbum { artist: String, album: String },
    EnqueueAlbum { artist: String, album: String },
    PlayQueueIndex(usize),
    ClearPlaylist,
    RemoveQueueIndex(usize),
    GroupBySelected(usize),
    PlayRadio { name: String, url: String },
    CustomName(String),
    CustomUrl(String),
    AddCustomStream,
    FilesEnter(PathBuf),
    FilesUp,
    FilesPlay(PathBuf),
    FilesEnqueue(PathBuf),
    FilesPlayCwd,
    LoadSavedPlaylist(i64),
    SmartNeverPlayed,
    SmartHighestRated,
    SmartMostPlayed,
    RepeatToggle,
    ShuffleToggle,
    EqBand { band: usize, db: f32 },
    Tick,
    Noop,
}

pub struct OrangeApp {
    core: Core,
    nav: nav_bar::Model,
    player: Player,
    collection: CollectionState,
    files: FileBrowser,
    appearance: AppearanceMode,
    volume: f32,
    status: String,
    custom_streams: Vec<CustomStream>,
    custom_name: String,
    custom_url: String,
    equalizer: Equalizer,
    repeat: RepeatMode,
    shuffle: ShuffleMode,
    phase: u32,
    position_secs: i64,
    spectrum: Vec<f32>,
    #[cfg(feature = "dbus")]
    mpris: Option<MprisShell>,
    #[cfg(feature = "gst")]
    engine: Option<GstEngine>,
    #[cfg(feature = "gst")]
    engine_url: String,
}

#[cfg(feature = "dbus")]
struct MprisShell {
    host: crate::mpris_host::MprisHost,
    commands: std::sync::mpsc::Receiver<MprisCommand>,
}

/// Launch the COSMIC window. `uris` are files/streams passed on the CLI.
pub fn run(uris: Vec<String>) -> Result<(), Box<dyn std::error::Error>> {
    prefer_breeze_icons();
    let settings = Settings::default().size(Size::new(1280.0, 800.0));
    cosmic::app::run::<OrangeApp>(settings, uris)?;
    Ok(())
}

fn prefer_breeze_icons() {
    if orange_theme::find_breeze_icon_dir(&orange_theme::system_icon_bases()).is_some() {
        cosmic::icon_theme::set_default("Breeze");
    }
}

fn songs_to_tracks(songs: impl IntoIterator<Item = Song>) -> Vec<QueuedTrack> {
    songs
        .into_iter()
        .map(|s| QueuedTrack::from_song(&s))
        .collect()
}

impl OrangeApp {
    fn active_page(&self) -> Page {
        self.nav
            .active_data::<Page>()
            .copied()
            .unwrap_or(Page::Collection)
    }

    fn now_playing_label(&self) -> String {
        match self.player.current() {
            Some(track) if track.title.is_empty() => track.url.clone(),
            Some(track) => {
                if track.artist.is_empty() {
                    track.title.clone()
                } else {
                    format!("{} — {}", track.artist, track.title)
                }
            }
            None => String::from("Not playing"),
        }
    }

    fn duration_secs(&self) -> i64 {
        self.player
            .current()
            .map(QueuedTrack::length_secs)
            .filter(|&s| s > 0)
            .unwrap_or(self.position_secs)
    }

    fn status_line(&self) -> String {
        let stats = self.collection.stats();
        format!(
            "{} · {} songs · {} albums · {} artists · {}",
            about::footer(),
            stats.songs,
            stats.albums,
            stats.artists,
            self.status
        )
    }

    fn enqueue_songs(&mut self, songs: Vec<Song>, play: bool) {
        if songs.is_empty() {
            self.status = String::from("Nothing to play.");
            return;
        }
        let start = self.player.queue_len();
        let tracks = songs_to_tracks(songs);
        let n = tracks.len();
        self.player.enqueue_many(tracks);
        if play {
            self.player.play_at(start);
            self.sync_engine();
        }
        self.status = format!("Queued {n} track(s).");
    }

    fn replace_songs(&mut self, songs: Vec<Song>) {
        if songs.is_empty() {
            self.status = String::from("Nothing to play.");
            return;
        }
        let n = songs.len();
        self.player.replace_and_play(songs_to_tracks(songs), 0);
        self.sync_engine();
        self.status = format!("Playing {n} track(s).");
    }

    fn play_url(&mut self, url: &str) {
        if let Some(song) = self.collection.song_by_url(url).cloned() {
            if let Some(idx) = self.player.queue().iter().position(|t| t.url == url) {
                self.player.play_at(idx);
            } else {
                let start = self.player.queue_len();
                self.player.enqueue(QueuedTrack::from_song(&song));
                self.player.play_at(start);
            }
            self.sync_engine();
            self.status = format!("Playing {}", song.display_title());
            return;
        }
        let title = orange_core::paths::url_file_stem(url);
        let start = self.player.queue_len();
        self.player.enqueue(QueuedTrack {
            url: url.to_string(),
            title,
            ..QueuedTrack::default()
        });
        self.player.play_at(start);
        self.sync_engine();
    }

    fn skip_next(&mut self) {
        match self.repeat {
            RepeatMode::Track => {
                if let Some(cursor) = self.player.cursor() {
                    self.player.play_at(cursor);
                }
            }
            _ => {
                if self.shuffle == ShuffleMode::All && self.player.queue_len() > 1 {
                    let len = self.player.queue_len();
                    let mut next = (self.phase as usize).wrapping_mul(17) % len;
                    if Some(next) == self.player.cursor() {
                        next = (next + 1) % len;
                    }
                    self.player.play_at(next);
                } else if !self.player.next() {
                    if self.repeat == RepeatMode::Playlist && self.player.queue_len() > 0 {
                        self.player.play_at(0);
                    }
                }
            }
        }
        self.sync_engine();
    }

    fn sync_engine(&mut self) {
        #[cfg(feature = "gst")]
        {
            match self.player.state() {
                EngineState::Playing => self.ensure_playing_engine(),
                EngineState::Paused => {
                    if let Some(engine) = self.engine.as_ref() {
                        let _ = engine.pause();
                    }
                }
                EngineState::Idle | EngineState::Empty => {
                    if let Some(engine) = self.engine.take() {
                        let _ = engine.stop();
                    }
                    self.engine_url.clear();
                    self.position_secs = 0;
                }
            }
        }
        #[cfg(not(feature = "gst"))]
        {
            if self.player.state() == EngineState::Playing {
                self.status = String::from("Playing (rebuild with gst for audio output).");
            }
        }
    }

    #[cfg(feature = "gst")]
    fn ensure_playing_engine(&mut self) {
        let Some(current) = self.player.current() else {
            return;
        };
        if self.engine_url == current.url {
            if let Some(engine) = self.engine.as_ref() {
                let _ = engine.play();
            }
            return;
        }
        if let Some(engine) = self.engine.take() {
            let _ = engine.stop();
        }
        let volume = self.player.volume();
        let software_volume = if volume >= 100 {
            None
        } else {
            Some(volume as f64 / 100.0)
        };
        let mut fx = FxChain {
            spectrum_bands: Some(16),
            ..FxChain::default()
        };
        let mut gains = [0.0; 10];
        let mut any_eq = false;
        for band in 0..Equalizer::BANDS {
            if let Some(db) = self.equalizer.gain(band) {
                gains[band] = db;
                if db.abs() > f64::EPSILON {
                    any_eq = true;
                }
            }
        }
        if any_eq {
            fx.equalizer_db = Some(gains);
        }
        let chain = PlaybackChain {
            uri: current.url.clone(),
            sink: AudioSink::Auto,
            fx,
        };
        match GstEngine::new_playback(
            &chain,
            software_volume,
            false,
            orange_media::audio_fx::NormalizationMode::Off,
        )
        .and_then(|engine| {
            engine.play()?;
            Ok(engine)
        }) {
            Ok(engine) => {
                self.engine_url = current.url.clone();
                self.engine = Some(engine);
            }
            Err(e) => {
                self.status = format!("Audio engine: {e}");
                self.engine_url.clear();
            }
        }
    }

    #[cfg(feature = "dbus")]
    fn poll_remotes(&mut self) {
        let commands: Vec<MprisCommand> = match self.mpris.as_mut() {
            Some(shell) => shell.commands.try_iter().collect(),
            None => return,
        };
        for command in &commands {
            if crate::mpris_host::apply_command(&mut self.player, command) {
                self.status = String::from("Remote quit: playback stopped.");
            }
        }
        self.sync_engine();
        let title = self.now_playing_label();
        let changed = match self.mpris.as_mut() {
            Some(shell) => {
                let volume = self.player.volume() as f64 / 100.0;
                shell
                    .host
                    .sync_player(&self.player, self.position_secs * 1_000_000, volume)
            }
            None => false,
        };
        if changed && self.player.current().is_some() {
            #[cfg(feature = "notify")]
            crate::notify::notify_track(&title, "", "");
            let _ = title;
        }
    }

    fn apply_appearance(&self) -> cosmic::app::Task<Message> {
        match self.appearance {
            AppearanceMode::Light => cosmic::command::set_theme(cosmic::Theme::light()),
            AppearanceMode::Dark => cosmic::command::set_theme(cosmic::Theme::dark()),
            AppearanceMode::System => {
                let system = self.core.system_theme().cosmic().clone();
                cosmic::command::set_theme(cosmic::Theme::system(std::sync::Arc::new(system)))
            }
        }
    }

    fn player_bar(&self) -> Element<'_, Message> {
        let playing = self.player.state() == EngineState::Playing;
        let play_icon = if playing {
            "media-playback-pause-symbolic"
        } else {
            "media-playback-start-symbolic"
        };
        let title = fit_title(&self.now_playing_label(), 42);
        let duration = self.duration_secs();
        let ratio = seek_ratio(self.position_secs, duration);
        let layout = PlayerBarLayout { width: 1100.0 };
        let spark = if layout.analyzer_visible() {
            let bars = analyzer_bars(playing, self.phase, &self.spectrum, 16);
            analyzer_sparkline(&bars)
        } else {
            String::new()
        };
        let repeat = match self.repeat {
            RepeatMode::Off => "Repeat: off",
            RepeatMode::Track => "Repeat: track",
            RepeatMode::Album => "Repeat: album",
            RepeatMode::Playlist => "Repeat: playlist",
        };
        let shuffle = match self.shuffle {
            ShuffleMode::Off => "Shuffle: off",
            ShuffleMode::All => "Shuffle: all",
            ShuffleMode::InsideAlbum => "Shuffle: album",
        };
        widget::column::with_capacity(2)
            .push(
                widget::row::with_capacity(12)
                    .push(
                        widget::button::icon(widget::icon::from_name(
                            "media-skip-backward-symbolic",
                        ))
                        .on_press(Message::Previous),
                    )
                    .push(
                        widget::button::icon(widget::icon::from_name(play_icon))
                            .on_press(Message::PlayPause),
                    )
                    .push(
                        widget::button::icon(widget::icon::from_name(
                            "media-playback-stop-symbolic",
                        ))
                        .on_press(Message::Stop),
                    )
                    .push(
                        widget::button::icon(widget::icon::from_name(
                            "media-skip-forward-symbolic",
                        ))
                        .on_press(Message::Next),
                    )
                    .push(widget::text(title).width(Length::Fill))
                    .push(widget::text(orange_core::song::format_duration_secs(
                        self.position_secs,
                    )))
                    .push(
                        widget::slider(0.0..=1.0, ratio, Message::Seek).width(Length::Fixed(180.0)),
                    )
                    .push(widget::text(orange_core::song::format_duration_secs(
                        duration,
                    )))
                    .push(widget::text(format!("{}%", self.volume as u8)))
                    .push(
                        widget::slider(0.0..=100.0, self.volume, Message::VolumeChanged)
                            .width(Length::Fixed(90.0)),
                    )
                    .push(widget::text(spark))
                    .spacing(8)
                    .padding(8)
                    .align_y(Alignment::Center),
            )
            .push(
                widget::row::with_capacity(3)
                    .push(widget::button::text(repeat).on_press(Message::RepeatToggle))
                    .push(widget::button::text(shuffle).on_press(Message::ShuffleToggle))
                    .spacing(8)
                    .padding(8),
            )
            .into()
    }

    fn source_pane(&self, page: Page) -> Element<'_, Message> {
        match page {
            Page::Collection => self.collection_pane(),
            Page::Playlists => self.playlists_pane(),
            Page::Files => self.files_pane(),
            Page::Radio => self.radio_pane(),
            Page::Devices => self.devices_pane(),
            Page::Settings => self.settings_page(),
        }
    }

    fn collection_pane(&self) -> Element<'_, Message> {
        let stats = self.collection.stats();
        let mut col = widget::column::with_capacity(8)
            .push(widget::text::heading("Collection"))
            .push(
                widget::text_input::search_input("Search collection", &self.collection.search)
                    .on_input(Message::SearchChanged),
            )
            .push(widget::dropdown(
                GroupBy::ALL.iter().map(|g| g.label()).collect::<Vec<_>>(),
                GroupBy::ALL
                    .iter()
                    .position(|g| *g == self.collection.group_by),
                Message::GroupBySelected,
            ))
            .push(widget::text::caption(format!(
                "{} songs · {} albums · {} artists",
                stats.songs, stats.albums, stats.artists
            )));

        col = col.push(
            widget::row::with_capacity(3)
                .push(
                    widget::text_input("Music folder path", &self.collection.add_path)
                        .on_input(Message::AddPathChanged),
                )
                .push(widget::button::suggested("Add folder").on_press(Message::AddFolder))
                .push(widget::button::standard("Rescan").on_press(Message::Rescan))
                .spacing(8),
        );

        if !self.collection.directories.is_empty() {
            let mut dirs = widget::column::with_capacity(self.collection.directories.len() + 1)
                .push(widget::text::caption("Music folders"));
            for dir in &self.collection.directories {
                dirs = dirs.push(
                    widget::row::with_capacity(2)
                        .push(widget::text(&dir.path).width(Length::Fill))
                        .push(
                            widget::button::destructive("Remove")
                                .on_press(Message::RemoveFolder(dir.id)),
                        )
                        .spacing(8)
                        .align_y(Alignment::Center),
                );
            }
            col = col.push(dirs);
        }

        if self.collection.songs.is_empty() {
            col = col
                .push(widget::text("Your collection is empty."))
                .push(widget::text::caption(
                    "Add a music folder (for example ~/Music) and Orange will scan it into the library. Files stay where they are; Orange never moves or deletes them, and never touches Strawberry’s data.",
                ));
        } else {
            let tree = self.collection.tree();
            let mut tree_col = widget::column::with_capacity(tree.len().saturating_mul(4) + 1);
            for artist in &tree {
                let expanded = self.collection.artist_expanded(&artist.name);
                let chevron = if expanded { "▾" } else { "▸" };
                let name = artist.name.clone();
                tree_col = tree_col.push(
                    widget::row::with_capacity(3)
                        .push(
                            widget::button::text(format!(
                                "{chevron} {} ({})",
                                artist.name,
                                artist.song_count()
                            ))
                            .on_press(Message::ToggleArtist(name.clone())),
                        )
                        .push(
                            widget::button::text("Play")
                                .on_press(Message::PlayArtist(name.clone())),
                        )
                        .push(
                            widget::button::text("Enqueue").on_press(Message::EnqueueArtist(name)),
                        )
                        .spacing(4),
                );
                if expanded {
                    for album in &artist.albums {
                        let album_expanded =
                            self.collection.album_expanded(&artist.name, &album.name);
                        let chevron = if album_expanded { "▾" } else { "▸" };
                        let year = if album.year > 0 {
                            format!(" ({})", album.year)
                        } else {
                            String::new()
                        };
                        let artist_name = artist.name.clone();
                        let album_name = album.name.clone();
                        tree_col = tree_col.push(
                            widget::row::with_capacity(3)
                                .push(
                                    widget::button::text(format!(
                                        "    {chevron} {}{year} ({})",
                                        album.name,
                                        album.songs.len()
                                    ))
                                    .on_press(
                                        Message::ToggleAlbum {
                                            artist: artist_name.clone(),
                                            album: album_name.clone(),
                                        },
                                    ),
                                )
                                .push(widget::button::text("Play").on_press(Message::PlayAlbum {
                                    artist: artist_name.clone(),
                                    album: album_name.clone(),
                                }))
                                .push(widget::button::text("Enqueue").on_press(
                                    Message::EnqueueAlbum {
                                        artist: artist_name,
                                        album: album_name,
                                    },
                                ))
                                .spacing(4),
                        );
                        if album_expanded {
                            for song in &album.songs {
                                let track = if song.track > 0 {
                                    format!("{:02}  ", song.track)
                                } else {
                                    String::from("    ")
                                };
                                let url = song.url.clone();
                                tree_col = tree_col.push(
                                    widget::row::with_capacity(2)
                                        .push(
                                            widget::button::text(format!(
                                                "        {track}{}",
                                                song.display_title()
                                            ))
                                            .on_press(Message::PlaySong(url.clone())),
                                        )
                                        .push(
                                            widget::button::text("+")
                                                .on_press(Message::EnqueueSong(url)),
                                        )
                                        .spacing(4),
                                );
                            }
                        }
                    }
                }
            }
            col = col.push(widget::scrollable(tree_col.spacing(2)).height(Length::Fill));
        }

        if let Some(err) = &self.collection.last_error {
            col = col.push(widget::text(format!("Error: {err}")));
        }
        if let Some(scan) = &self.collection.last_scan {
            col = col.push(widget::text::caption(format!(
                "Last scan: {} songs ({} added, {} removed) in {}",
                scan.songs, scan.added, scan.removed, scan.path
            )));
        }

        widget::container(col.spacing(8).padding(8))
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }

    fn playlists_pane(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(8)
            .push(widget::text::heading("Playlists"))
            .push(widget::text::caption(format!(
                "Current playlist: {} track(s).",
                self.player.queue_len()
            )))
            .push(
                widget::row::with_capacity(3)
                    .push(widget::button::text("Never played").on_press(Message::SmartNeverPlayed))
                    .push(
                        widget::button::text("Highest rated").on_press(Message::SmartHighestRated),
                    )
                    .push(widget::button::text("Most played").on_press(Message::SmartMostPlayed))
                    .spacing(8),
            )
            .push(widget::text::caption(
                "Smart playlists generate from the collection. Saved playlists from the Orange database appear below.",
            ));
        if self.collection.playlists.is_empty() {
            col = col.push(widget::text::caption("No saved playlists yet."));
        } else {
            for list in &self.collection.playlists {
                let star = if list.favorite { "★ " } else { "" };
                col = col.push(
                    widget::button::text(format!("{star}{}", list.name))
                        .on_press(Message::LoadSavedPlaylist(list.id)),
                );
            }
        }
        widget::container(widget::scrollable(col.spacing(8).padding(8)))
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }

    fn files_pane(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(6)
            .push(widget::text::heading("Files"))
            .push(widget::text::caption(self.files.cwd.display().to_string()))
            .push(
                widget::row::with_capacity(2)
                    .push(widget::button::text("Up").on_press(Message::FilesUp))
                    .push(widget::button::text("Play folder").on_press(Message::FilesPlayCwd))
                    .spacing(8),
            );
        if let Some(err) = &self.files.error {
            col = col.push(widget::text(format!("Error: {err}")));
        }
        let mut list = widget::column::with_capacity(self.files.entries.len().max(1));
        for entry in &self.files.entries {
            if entry.is_dir {
                list = list.push(
                    widget::button::text(format!("📁 {}", entry.name))
                        .on_press(Message::FilesEnter(entry.path.clone())),
                );
            } else {
                list = list.push(
                    widget::row::with_capacity(2)
                        .push(
                            widget::button::text(format!("♪ {}", entry.name))
                                .on_press(Message::FilesPlay(entry.path.clone())),
                        )
                        .push(
                            widget::button::text("+")
                                .on_press(Message::FilesEnqueue(entry.path.clone())),
                        )
                        .spacing(4),
                );
            }
        }
        col = col.push(widget::scrollable(list.spacing(2)).height(Length::Fill));
        widget::container(col.spacing(8).padding(8))
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }

    fn radio_pane(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(8)
            .push(widget::text::heading("Internet radio"))
            .push(widget::text::caption(
                "Radio Paradise · SomaFM · custom streams. Click a channel to play.",
            ));
        for service in [radio_paradise(), somafm()] {
            col = col.push(widget::text::heading(service.name));
            for stream in service.streams {
                col = col.push(widget::button::text(stream.name.clone()).on_press(
                    Message::PlayRadio {
                        name: format!("{} — {}", service.name, stream.name),
                        url: stream.url,
                    },
                ));
            }
        }
        col = col
            .push(widget::text::heading("Your streams"))
            .push(widget::text_input("Name", &self.custom_name).on_input(Message::CustomName))
            .push(widget::text_input("https://…", &self.custom_url).on_input(Message::CustomUrl))
            .push(widget::button::suggested("Add stream").on_press(Message::AddCustomStream));
        for stream in &self.custom_streams {
            col = col.push(widget::button::text(stream.name.clone()).on_press(
                Message::PlayRadio {
                    name: stream.name.clone(),
                    url: stream.url.clone(),
                },
            ));
        }
        widget::container(widget::scrollable(col.spacing(8).padding(8)))
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }

    fn devices_pane(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(8)
            .push(widget::text::heading("Devices"))
            .push(widget::text::caption(
                "USB mass-storage, MTP, and iPod Classic. Connect a player to copy music from the playlist. Orange never writes into Strawberry’s device databases.",
            ));
        for family in [
            DeviceFamily::UsbMassStorage,
            DeviceFamily::Mtp,
            DeviceFamily::IPod,
        ] {
            col = col.push(widget::text(format!("• {} — supported", family.name())));
        }
        col = col.push(widget::text::caption(
            "No removable device is connected in this session. Plug in a USB / MTP / iPod player and reopen Devices to sync.",
        ));
        widget::container(col.spacing(8).padding(8))
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }

    fn playlist_pane(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(4)
            .push(
                widget::row::with_capacity(3)
                    .push(widget::text::heading("Playlist").width(Length::Fill))
                    .push(widget::button::text("Clear").on_press(Message::ClearPlaylist))
                    .spacing(8)
                    .align_y(Alignment::Center),
            )
            .push(
                widget::row::with_capacity(6)
                    .push(widget::text("#").width(Length::Fixed(36.0)))
                    .push(widget::text("Title").width(Length::Fill))
                    .push(widget::text("Artist").width(Length::Fill))
                    .push(widget::text("Album").width(Length::Fill))
                    .push(widget::text("Time").width(Length::Fixed(56.0)))
                    .push(widget::text("").width(Length::Fixed(28.0)))
                    .spacing(8),
            );
        let cursor = self.player.cursor();
        let mut rows = widget::column::with_capacity(self.player.queue_len().max(1));
        if self.player.queue().is_empty() {
            rows = rows.push(widget::text::caption(
                "The playlist is empty. Play a song from Collection, Files, or Radio.",
            ));
        } else {
            for (index, track) in self.player.queue().iter().enumerate() {
                let marker = if Some(index) == cursor { "▶" } else { "" };
                let number = if track.track > 0 {
                    format!("{}", track.track)
                } else {
                    format!("{}", index + 1)
                };
                rows = rows.push(
                    widget::row::with_capacity(6)
                        .push(
                            widget::button::text(format!("{marker}{number}"))
                                .on_press(Message::PlayQueueIndex(index))
                                .width(Length::Fixed(36.0)),
                        )
                        .push(
                            widget::button::text(track.title.clone())
                                .on_press(Message::PlayQueueIndex(index))
                                .width(Length::Fill),
                        )
                        .push(widget::text(track.artist.clone()).width(Length::Fill))
                        .push(widget::text(track.album.clone()).width(Length::Fill))
                        .push(widget::text(track.format_length()).width(Length::Fixed(56.0)))
                        .push(
                            widget::button::text("×")
                                .on_press(Message::RemoveQueueIndex(index))
                                .width(Length::Fixed(28.0)),
                        )
                        .spacing(8)
                        .align_y(Alignment::Center),
                );
            }
        }
        col = col.push(widget::scrollable(rows.spacing(2)).height(Length::Fill));
        widget::container(col.spacing(8).padding(8))
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }

    fn context_pane(&self) -> Element<'_, Message> {
        let (title, artist, album, lyrics) = match self.player.current() {
            Some(track) => {
                let lyrics = self
                    .collection
                    .song_by_url(&track.url)
                    .map(|s| s.lyrics.clone())
                    .filter(|l| !l.trim().is_empty())
                    .unwrap_or_else(|| {
                        String::from(
                            "No lyrics stored for this track. Providers (Genius, Musixmatch, lrclib, lyrics.ovh) fetch when configured.",
                        )
                    });
                (
                    track.title.clone(),
                    track.artist.clone(),
                    track.album.clone(),
                    lyrics,
                )
            }
            None => (
                String::from("Not playing"),
                String::new(),
                String::new(),
                String::from("Lyrics appear here when a track is playing."),
            ),
        };
        widget::container(
            widget::scrollable(
                widget::column::with_capacity(8)
                    .push(widget::text::heading("Now playing"))
                    .push(widget::text(title))
                    .push(widget::text(artist))
                    .push(widget::text(album))
                    .push(widget::text::heading("Lyrics"))
                    .push(widget::text(lyrics))
                    .spacing(8)
                    .padding(8),
            )
            .height(Length::Fill),
        )
        .width(Length::Fill)
        .height(Length::Fill)
        .into()
    }

    fn settings_page(&self) -> Element<'_, Message> {
        let mode_button = |mode: AppearanceMode, label: &'static str| {
            widget::button::text(label).on_press(Message::AppearanceSelected(mode))
        };
        let mut eq = widget::column::with_capacity(Equalizer::BANDS + 1)
            .push(widget::text::heading("Equalizer (10-band)"));
        for band in 0..Equalizer::BANDS {
            let db = self.equalizer.gain(band).unwrap_or(0.0) as f32;
            let freq = Equalizer::FREQUENCIES_HZ[band];
            eq = eq.push(
                widget::row::with_capacity(3)
                    .push(widget::text(format!("{freq} Hz")).width(Length::Fixed(80.0)))
                    .push(widget::slider(-12.0..=12.0, db, move |value| {
                        Message::EqBand { band, db: value }
                    }))
                    .push(widget::text(format!("{db:.1} dB")).width(Length::Fixed(64.0)))
                    .spacing(8)
                    .align_y(Alignment::Center),
            );
        }
        let codecs = orange_core::codecs::SUPPORTED_CODECS
            .iter()
            .map(|c| c.name)
            .collect::<Vec<_>>()
            .join(", ");
        let targets = orange_core::codecs::TRANSCODE_TARGETS
            .iter()
            .map(|t| t.name)
            .collect::<Vec<_>>()
            .join(", ");
        widget::scrollable(
            widget::column::with_capacity(10)
                .push(widget::text::heading("Appearance"))
                .push(
                    widget::row::with_capacity(3)
                        .push(mode_button(AppearanceMode::System, "System"))
                        .push(mode_button(AppearanceMode::Light, "Light"))
                        .push(mode_button(AppearanceMode::Dark, "Dark"))
                        .spacing(8),
                )
                .push(widget::text::caption(format!(
                    "Active: {}",
                    self.appearance.as_str()
                )))
                .push(widget::text::heading("Collection"))
                .push(widget::text::caption(
                    "Manage music folders from the Collection tab: add a path, scan, and browse Album artist → Album → tracks. Orange writes only ~/.local/share/orange/orange/orange.db.",
                ))
                .push(eq.spacing(4))
                .push(widget::text::heading("Playback"))
                .push(widget::text::caption(
                    "Bit-perfect by default (no resample or software volume unless you move the volume slider or equalizer).",
                ))
                .push(widget::text::heading("Formats"))
                .push(widget::text::caption(format!("Plays: {codecs}")))
                .push(widget::text::caption(format!("Transcode to: {targets}")))
                .push(widget::text::heading("About"))
                .push(widget::text(about::title()))
                .push(widget::text(about::maker_line()))
                .push(widget::text(about::body()))
                .spacing(10)
                .padding(16),
        )
        .into()
    }
}

fn analyzer_sparkline(bars: &[f32]) -> String {
    const GLYPHS: [char; 8] = ['▁', '▂', '▃', '▄', '▅', '▆', '▇', '█'];
    bars.iter()
        .map(|h| {
            let idx = ((h * 7.0).round() as usize).min(7);
            GLYPHS[idx]
        })
        .collect()
}

impl Application for OrangeApp {
    type Executor = executor::Default;
    type Flags = Vec<String>;
    type Message = Message;

    const APP_ID: &'static str = "com.goshapps.Orange";

    fn core(&self) -> &Core {
        &self.core
    }

    fn core_mut(&mut self) -> &mut Core {
        &mut self.core
    }

    fn init(core: Core, uris: Self::Flags) -> (Self, cosmic::app::Task<Self::Message>) {
        let mut nav = nav_bar::Model::default();
        for page in Page::ALL {
            nav.insert()
                .text(page.title())
                .icon(widget::icon::from_name(orange_theme::nav_icon_name(
                    page.icon_key(),
                )))
                .data(*page);
        }
        nav.activate_position(0);
        #[cfg(feature = "dbus")]
        let mpris = {
            let host = crate::mpris_host::MprisHost::new();
            let (tx, commands) = std::sync::mpsc::channel();
            host.spawn_server(BUS_NAME.to_string(), tx);
            Some(MprisShell { host, commands })
        };
        let mut player = Player::new();
        for uri in uris {
            let title = orange_core::paths::url_file_stem(&uri);
            player.enqueue(QueuedTrack {
                url: uri,
                title,
                ..QueuedTrack::default()
            });
        }
        if player.queue_len() > 0 {
            player.play_at(0);
        }
        let mut app = Self {
            core,
            nav,
            player,
            collection: CollectionState::open(),
            files: FileBrowser::default(),
            appearance: AppearanceMode::System,
            volume: 100.0,
            status: String::from("Ready."),
            custom_streams: Vec::new(),
            custom_name: String::new(),
            custom_url: String::new(),
            equalizer: Equalizer::flat(),
            repeat: RepeatMode::Off,
            shuffle: ShuffleMode::Off,
            phase: 0,
            position_secs: 0,
            spectrum: Vec::new(),
            #[cfg(feature = "dbus")]
            mpris,
            #[cfg(feature = "gst")]
            engine: None,
            #[cfg(feature = "gst")]
            engine_url: String::new(),
        };
        if app.player.state() == EngineState::Playing {
            app.sync_engine();
        }
        if let Some(err) = &app.collection.last_error {
            app.status = format!("Collection: {err}");
        } else if app.collection.songs.is_empty() {
            app.status = String::from("Add a music folder in Collection to build your library.");
        } else {
            let stats = app.collection.stats();
            app.status = format!("Loaded {} songs.", stats.songs);
        }
        (app, cosmic::app::Task::none())
    }

    fn nav_model(&self) -> Option<&nav_bar::Model> {
        Some(&self.nav)
    }

    fn on_nav_select(&mut self, id: nav_bar::Id) -> cosmic::app::Task<Self::Message> {
        self.nav.activate(id);
        cosmic::app::Task::none()
    }

    fn header_end(&self) -> Vec<Element<'_, Self::Message>> {
        vec![widget::button::standard("Rescan")
            .on_press(Message::Rescan)
            .into()]
    }

    fn footer(&self) -> Option<Element<'_, Self::Message>> {
        Some(widget::text::caption(self.status_line()).into())
    }

    fn subscription(&self) -> cosmic::iced::Subscription<Self::Message> {
        cosmic::iced::time::every(Duration::from_millis(100)).map(|_| Message::Tick)
    }

    fn update(&mut self, message: Self::Message) -> cosmic::app::Task<Self::Message> {
        #[cfg(feature = "dbus")]
        self.poll_remotes();
        match message {
            Message::PlayPause => {
                self.player.toggle_play_pause();
                self.status = format!("State: {:?}", self.player.state());
                self.sync_engine();
            }
            Message::Stop => {
                self.player.stop();
                self.position_secs = 0;
                self.sync_engine();
                self.status = String::from("Stopped.");
            }
            Message::Next => {
                self.skip_next();
                self.status = String::from("Next track.");
            }
            Message::Previous => {
                if !self.player.previous() {
                    if let Some(cursor) = self.player.cursor() {
                        self.player.play_at(cursor);
                    }
                }
                self.sync_engine();
                self.status = String::from("Previous track.");
            }
            Message::Seek(ratio) => {
                let duration = self.duration_secs();
                self.position_secs = (ratio.clamp(0.0, 1.0) * duration as f32) as i64;
                #[cfg(feature = "gst")]
                if let Some(engine) = self.engine.as_ref() {
                    let _ = engine.seek_secs(self.position_secs.max(0) as u64);
                }
            }
            Message::VolumeChanged(volume) => {
                self.volume = volume.clamp(0.0, 100.0);
                self.player.set_volume(self.volume as u8);
                self.sync_engine();
            }
            Message::AppearanceSelected(mode) => {
                self.appearance = mode;
                return self.apply_appearance();
            }
            Message::SearchChanged(search) => {
                self.collection.search = search;
            }
            Message::AddPathChanged(path) => {
                self.collection.add_path = path;
            }
            Message::AddFolder => {
                let path = self.collection.add_path.clone();
                match self.collection.add_folder(&path) {
                    Ok(report) => {
                        self.status = format!("Added {} ({} songs).", report.path, report.songs);
                    }
                    Err(e) => self.status = e,
                }
            }
            Message::RemoveFolder(id) => match self.collection.remove_folder(id) {
                Ok(()) => self.status = String::from("Removed music folder."),
                Err(e) => self.status = e,
            },
            Message::Rescan => match self.collection.rescan_all() {
                Ok(report) => {
                    self.status = format!(
                        "Rescan complete: {} songs ({} added, {} removed).",
                        report.songs, report.added, report.removed
                    );
                }
                Err(e) => self.status = e,
            },
            Message::ToggleArtist(name) => self.collection.toggle_artist(&name),
            Message::ToggleAlbum { artist, album } => {
                self.collection.toggle_album(&artist, &album);
            }
            Message::PlaySong(url) => self.play_url(&url),
            Message::EnqueueSong(url) => {
                if let Some(song) = self.collection.song_by_url(&url).cloned() {
                    self.enqueue_songs(vec![song], self.player.state() != EngineState::Playing);
                }
            }
            Message::PlayArtist(name) => {
                let songs = self.collection.songs_for_artist(&name);
                self.replace_songs(songs);
            }
            Message::EnqueueArtist(name) => {
                let songs = self.collection.songs_for_artist(&name);
                self.enqueue_songs(songs, false);
            }
            Message::PlayAlbum { artist, album } => {
                let songs = self.collection.songs_for_album(&artist, &album);
                self.replace_songs(songs);
            }
            Message::EnqueueAlbum { artist, album } => {
                let songs = self.collection.songs_for_album(&artist, &album);
                self.enqueue_songs(songs, false);
            }
            Message::PlayQueueIndex(index) => {
                if self.player.play_at(index) {
                    self.sync_engine();
                }
            }
            Message::ClearPlaylist => {
                self.player.clear_queue();
                self.sync_engine();
                self.status = String::from("Playlist cleared.");
            }
            Message::RemoveQueueIndex(index) => {
                self.player.remove_at(index);
            }
            Message::GroupBySelected(index) => {
                if let Some(group) = GroupBy::ALL.get(index) {
                    self.collection.group_by = *group;
                }
            }
            Message::PlayRadio { name, url } => {
                let start = self.player.queue_len();
                self.player.enqueue(QueuedTrack {
                    url,
                    title: name.clone(),
                    artist: String::from("Radio"),
                    ..QueuedTrack::default()
                });
                self.player.play_at(start);
                self.sync_engine();
                self.status = format!("Playing {name}");
            }
            Message::CustomName(name) => self.custom_name = name,
            Message::CustomUrl(url) => self.custom_url = url,
            Message::AddCustomStream => {
                match validate_custom_stream(&self.custom_name, &self.custom_url) {
                    Some(stream) => {
                        self.status = format!("Added stream {}", stream.name);
                        self.custom_name.clear();
                        self.custom_url.clear();
                        self.custom_streams.push(stream);
                    }
                    None => {
                        self.status = String::from("Enter a name and an http(s) stream URL.");
                    }
                }
            }
            Message::FilesEnter(path) => {
                if path.is_dir() {
                    self.files.enter(path);
                }
            }
            Message::FilesUp => self.files.go_up(),
            Message::FilesPlay(path) => {
                let song = orange_collection::scan::song_from_path(&path, &self.files.cwd, -1);
                self.replace_songs(vec![song]);
            }
            Message::FilesEnqueue(path) => {
                let song = orange_collection::scan::song_from_path(&path, &self.files.cwd, -1);
                self.enqueue_songs(vec![song], false);
            }
            Message::FilesPlayCwd => {
                let songs = self.files.audio_in_cwd();
                self.replace_songs(songs);
            }
            Message::LoadSavedPlaylist(id) => match self.collection.load_saved_playlist(id) {
                Ok(songs) => self.replace_songs(songs),
                Err(e) => self.status = e,
            },
            Message::SmartNeverPlayed => {
                self.replace_songs(smart_never_played(&self.collection.songs));
            }
            Message::SmartHighestRated => {
                self.replace_songs(smart_highest_rated(&self.collection.songs));
            }
            Message::SmartMostPlayed => {
                self.replace_songs(smart_most_played(&self.collection.songs));
            }
            Message::RepeatToggle => {
                self.repeat = match self.repeat {
                    RepeatMode::Off => RepeatMode::Playlist,
                    RepeatMode::Playlist => RepeatMode::Track,
                    RepeatMode::Track => RepeatMode::Album,
                    RepeatMode::Album => RepeatMode::Off,
                };
            }
            Message::ShuffleToggle => {
                self.shuffle = match self.shuffle {
                    ShuffleMode::Off => ShuffleMode::All,
                    ShuffleMode::All => ShuffleMode::InsideAlbum,
                    ShuffleMode::InsideAlbum => ShuffleMode::Off,
                };
            }
            Message::EqBand { band, db } => {
                self.equalizer.set_gain(band, db as f64);
                self.sync_engine();
            }
            Message::Tick => {
                self.phase = self.phase.wrapping_add(1);
                #[cfg(feature = "gst")]
                {
                    let mut ended = false;
                    if let Some(engine) = self.engine.as_ref() {
                        if let Some(pos) = engine.position_nanos() {
                            self.position_secs = (pos / 1_000_000_000) as i64;
                        }
                        match engine.poll_event(Duration::from_millis(0)) {
                            Some(EngineEvent::Eos) => ended = true,
                            Some(EngineEvent::Error(e)) => {
                                self.status = format!("Engine: {e}");
                            }
                            Some(EngineEvent::SpectrumTick { magnitudes_db }) => {
                                self.spectrum = magnitudes_db
                                    .iter()
                                    .map(|db| ((db + 60.0) / 60.0).clamp(0.04, 1.0))
                                    .collect();
                            }
                            _ => {}
                        }
                    }
                    if ended {
                        if self.player.track_ended().is_some() {
                            self.sync_engine();
                        } else if self.repeat == RepeatMode::Playlist && self.player.queue_len() > 0
                        {
                            self.player.play_at(0);
                            self.sync_engine();
                        } else {
                            self.sync_engine();
                        }
                    }
                }
            }
            Message::Noop => {}
        }
        cosmic::app::Task::none()
    }

    fn view(&self) -> Element<'_, Self::Message> {
        let page = self.active_page();
        if page == Page::Settings {
            return widget::column::with_capacity(2)
                .push(self.settings_page())
                .push(self.player_bar())
                .into();
        }
        widget::column::with_capacity(2)
            .push(
                widget::row::with_capacity(3)
                    .push(self.source_pane(page).width(Length::FillPortion(3)))
                    .push(self.playlist_pane().width(Length::FillPortion(5)))
                    .push(self.context_pane().width(Length::FillPortion(2)))
                    .height(Length::Fill),
            )
            .push(self.player_bar())
            .into()
    }
}
