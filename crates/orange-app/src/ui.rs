//! Rhythmbox window: source list, Genre/Artist/Album browser, track table,
//! top transport. No COSMIC nav bar or header chrome.

use std::path::PathBuf;
use std::time::Duration;

use cosmic::app::Settings;
use cosmic::iced::{Alignment, Length, Size};
use cosmic::{executor, widget, Application, ApplicationExt, Core, Element};

use orange_core::appearance::AppearanceMode;
use orange_core::song::Song;
use orange_media::audio_fx::Equalizer;
use orange_media::devices::DeviceFamily;
use orange_media::playback::{EngineState, Player, QueuedTrack};
use orange_media::radio::{radio_paradise, somafm, validate_custom_stream, CustomStream};
use orange_playlist::model::{RepeatMode, ShuffleMode};

use crate::about;
use crate::files::FileBrowser;
use crate::library::{
    smart_highest_rated, smart_most_played, smart_never_played, track_list_summary, CollectionState,
};
use crate::nav::Page;
use crate::playerbar::{fit_title, seek_ratio};

#[cfg(feature = "gst")]
use orange_media::backend::{AudioSink, FxChain, PlaybackChain};
#[cfg(feature = "gst")]
use orange_media::backend_gst::{EngineEvent, GstEngine};
#[cfg(feature = "dbus")]
use orange_media::mpris::BUS_NAME;
#[cfg(feature = "dbus")]
use orange_media::mpris_server::MprisCommand;

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
    SelectGenre(Option<String>),
    SelectArtist(Option<String>),
    SelectAlbum(Option<String>),
    SelectPage(Page),
    SelectVisibleIndex(usize),
    PlayVisibleIndex(usize),
    EnqueueVisibleIndex(usize),
    PlayQueueIndex(usize),
    ClearQueue,
    RemoveQueueIndex(usize),
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
    ToggleLyrics,
    EqBand { band: usize, db: f32 },
    Tick,
    Noop,
}

pub struct OrangeApp {
    core: Core,
    page: Page,
    selected_index: Option<usize>,
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
    #[allow(dead_code)]
    spectrum: Vec<f32>,
    show_lyrics: bool,
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

pub fn run(uris: Vec<String>) -> Result<(), Box<dyn std::error::Error>> {
    prefer_breeze_icons();
    let settings = Settings::default()
        .size(Size::new(1100.0, 720.0))
        .client_decorations(false)
        .transparent(false);
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
        self.page
    }

    fn now_playing_label(&self) -> String {
        match self.player.current() {
            Some(track) if track.title.is_empty() => track.url.clone(),
            Some(track) if track.artist.is_empty() => track.title.clone(),
            Some(track) => format!("{} — {}", track.artist, track.title),
            None => String::from("Not playing"),
        }
    }

    fn duration_secs(&self) -> i64 {
        let tagged = self
            .player
            .current()
            .map(QueuedTrack::length_secs)
            .unwrap_or(0);
        if tagged > 0 {
            return tagged;
        }
        #[cfg(feature = "gst")]
        if let Some(engine) = self.engine.as_ref() {
            if let Some(ns) = engine.duration_nanos() {
                return (ns / 1_000_000_000) as i64;
            }
        }
        0
    }

    fn enqueue_songs(&mut self, songs: Vec<Song>, play: bool) {
        if songs.is_empty() {
            self.status = String::from("Nothing to play.");
            return;
        }
        let start = self.player.queue_len();
        let n = songs.len();
        self.player.enqueue_many(songs_to_tracks(songs));
        if play {
            self.player.play_at(start);
            self.sync_engine();
        }
        self.status = format!("Queued {n} track(s).");
    }

    fn replace_songs_at(&mut self, songs: Vec<Song>, index: usize) {
        if songs.is_empty() {
            self.status = String::from("Nothing to play.");
            return;
        }
        let n = songs.len();
        self.player
            .replace_and_play(songs_to_tracks(songs), index.min(n.saturating_sub(1)));
        self.sync_engine();
        self.status = format!("Playing {n} track(s).");
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
                } else if !self.player.next()
                    && self.repeat == RepeatMode::Playlist
                    && self.player.queue_len() > 0
                {
                    self.player.play_at(0);
                }
            }
        }
        self.sync_engine();
    }

    fn play_or_resume(&mut self) {
        match self.player.state() {
            EngineState::Playing | EngineState::Paused => {
                self.player.toggle_play_pause();
                self.sync_engine();
            }
            EngineState::Idle | EngineState::Empty => {
                if self.player.queue_len() > 0 {
                    self.player.toggle_play_pause();
                    self.sync_engine();
                    return;
                }
                let tracks = self.collection.visible_tracks();
                if tracks.is_empty() {
                    self.status = String::from("Nothing to play.");
                    return;
                }
                let index = self
                    .selected_index
                    .unwrap_or(0)
                    .min(tracks.len().saturating_sub(1));
                self.replace_songs_at(tracks, index);
            }
        }
    }

    fn status_text(&self) -> String {
        let summary = match self.page {
            Page::Library => track_list_summary(&self.collection.visible_tracks()),
            Page::Queue => format!("{} in queue", self.player.queue_len()),
            Page::Playlists => format!("{} playlists", self.collection.playlists.len()),
            _ => String::new(),
        };
        if summary.is_empty() || summary == self.status {
            self.status.clone()
        } else if self.status.is_empty() {
            summary
        } else {
            format!("{summary} — {}", self.status)
        }
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
        let software_volume = Some(volume as f64 / 100.0);
        let mut fx = FxChain::default();
        let mut gains = [0.0; 10];
        let mut any_eq = false;
        for (band, gain) in gains.iter_mut().enumerate().take(Equalizer::BANDS) {
            if let Some(db) = self.equalizer.gain(band) {
                *gain = db;
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

    fn player_strip(&self) -> Element<'_, Message> {
        let playing = self.player.state() == EngineState::Playing;
        let play_icon = if playing {
            "media-playback-pause-symbolic"
        } else {
            "media-playback-start-symbolic"
        };
        let title = fit_title(&self.now_playing_label(), 56);
        let duration = self.duration_secs();
        let ratio = seek_ratio(self.position_secs, duration);
        let elapsed = orange_core::song::format_duration_secs(self.position_secs);
        let total = orange_core::song::format_duration_secs(duration);
        let repeat = match self.repeat {
            RepeatMode::Off => "Repeat",
            RepeatMode::Track => "Repeat one",
            RepeatMode::Album => "Repeat album",
            RepeatMode::Playlist => "Repeat all",
        };
        let shuffle = match self.shuffle {
            ShuffleMode::Off => "Shuffle",
            ShuffleMode::All => "Shuffle on",
            ShuffleMode::InsideAlbum => "Shuffle album",
        };
        widget::column::with_capacity(2)
            .push(
                widget::row::with_capacity(8)
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
                            "media-skip-forward-symbolic",
                        ))
                        .on_press(Message::Next),
                    )
                    .push(widget::text(title).width(Length::Fill))
                    .push(widget::button::text(repeat).on_press(Message::RepeatToggle))
                    .push(widget::button::text(shuffle).on_press(Message::ShuffleToggle))
                    .push(widget::button::text("Lyrics").on_press(Message::ToggleLyrics))
                    .push(widget::text(format!("{}%", self.volume as u8)))
                    .push(
                        widget::slider(0.0..=100.0, self.volume, Message::VolumeChanged)
                            .width(Length::Fixed(90.0)),
                    )
                    .spacing(6)
                    .align_y(Alignment::Center),
            )
            .push(
                widget::row::with_capacity(3)
                    .push(widget::text(elapsed).width(Length::Fixed(48.0)))
                    .push(widget::slider(0.0..=1.0, ratio, Message::Seek).width(Length::Fill))
                    .push(widget::text(total).width(Length::Fixed(48.0)))
                    .spacing(8)
                    .align_y(Alignment::Center),
            )
            .spacing(4)
            .padding(8)
            .into()
    }

    fn source_list(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(16).push(widget::text::heading("Library"));
        for page in [Page::Library, Page::Queue] {
            col = col.push(Self::select_row(
                page.title().to_string(),
                self.page == page,
                Message::SelectPage(page),
            ));
        }
        col = col
            .push(widget::text::heading("Playlists"))
            .push(Self::select_row(
                Page::Playlists.title().to_string(),
                self.page == Page::Playlists,
                Message::SelectPage(Page::Playlists),
            ));
        for list in &self.collection.playlists {
            let star = if list.favorite { "★ " } else { "" };
            col = col.push(Self::select_row(
                format!("{star}{}", list.name),
                false,
                Message::LoadSavedPlaylist(list.id),
            ));
        }
        col = col.push(widget::text::heading("Other"));
        for page in [Page::Radio, Page::Files, Page::Devices, Page::Settings] {
            col = col.push(Self::select_row(
                page.title().to_string(),
                self.page == page,
                Message::SelectPage(page),
            ));
        }
        widget::container(widget::scrollable(col.spacing(2).padding(8)))
            .width(Length::Fixed(180.0))
            .height(Length::Fill)
            .into()
    }

    fn browser_column(
        title: &'static str,
        selected: Option<String>,
        rows: Vec<(String, usize)>,
        all: Message,
        pick: impl Fn(String) -> Message,
    ) -> Element<'static, Message> {
        let total: usize = rows.iter().map(|(_, n)| *n).sum();
        let mut col = widget::column::with_capacity(rows.len() + 2)
            .push(widget::text::heading(title))
            .push(Self::select_row(
                format!("All ({total})"),
                selected.is_none(),
                all,
            ));
        for (name, count) in rows {
            let is_sel = selected.as_deref() == Some(name.as_str());
            col = col.push(Self::select_row(
                format!("{name} ({count})"),
                is_sel,
                pick(name),
            ));
        }
        widget::container(widget::scrollable(col.spacing(2)).height(Length::Fill))
            .width(Length::Fill)
            .height(Length::Fill)
            .into()
    }

    fn select_row(label: String, selected: bool, message: Message) -> Element<'static, Message> {
        if selected {
            widget::button::suggested(label).on_press(message).into()
        } else {
            widget::button::text(label).on_press(message).into()
        }
    }

    fn song_table(
        songs: Vec<Song>,
        cursor_url: Option<String>,
        selected: Option<usize>,
        play: impl Fn(usize) -> Message,
        select: impl Fn(usize) -> Message,
        extra_label: &'static str,
        extra: impl Fn(usize) -> Message,
    ) -> Element<'static, Message> {
        let mut header = widget::row::with_capacity(6)
            .push(widget::text("#").width(Length::Fixed(36.0)))
            .push(widget::text("Title").width(Length::Fill))
            .push(widget::text("Artist").width(Length::Fill))
            .push(widget::text("Album").width(Length::Fill))
            .push(widget::text("Time").width(Length::Fixed(56.0)));
        if !extra_label.is_empty() {
            header = header.push(widget::text("").width(Length::Fixed(28.0)));
        }
        let mut col = widget::column::with_capacity(songs.len() + 2).push(header.spacing(8));
        if songs.is_empty() {
            col = col.push(widget::text::caption("No songs in this view."));
        } else {
            for (index, song) in songs.into_iter().enumerate() {
                let marker = if cursor_url.as_deref() == Some(song.url.as_str()) {
                    "▶"
                } else if selected == Some(index) {
                    "•"
                } else {
                    ""
                };
                let number = if song.track > 0 {
                    format!("{marker}{}", song.track)
                } else {
                    format!("{marker}{}", index + 1)
                };
                let mut row = widget::row::with_capacity(6)
                    .push(widget::text(number).width(Length::Fixed(36.0)))
                    .push(widget::text(song.display_title()).width(Length::Fill))
                    .push(widget::text(song.display_artist().to_string()).width(Length::Fill))
                    .push(widget::text(song.display_album().to_string()).width(Length::Fill))
                    .push(widget::text(song.format_length()).width(Length::Fixed(56.0)));
                if !extra_label.is_empty() {
                    row = row.push(
                        widget::button::text(extra_label)
                            .on_press(extra(index))
                            .width(Length::Fixed(28.0)),
                    );
                }
                col = col.push(
                    widget::mouse_area(row.spacing(8).padding(2).align_y(Alignment::Center))
                        .on_press(select(index))
                        .on_double_click(play(index)),
                );
            }
        }
        widget::scrollable(col.spacing(2))
            .height(Length::Fill)
            .into()
    }

    fn library_page(&self) -> Element<'_, Message> {
        if self.collection.songs.is_empty() {
            return widget::column::with_capacity(6)
                .push(widget::text::heading("Music library"))
                .push(widget::text(
                    "The library is empty. Add a music folder to import tracks. Files stay where they are.",
                ))
                .push(
                    widget::row::with_capacity(3)
                        .push(
                            widget::text_input("Music folder path", &self.collection.add_path)
                                .on_input(Message::AddPathChanged),
                        )
                        .push(widget::button::suggested("Import").on_press(Message::AddFolder))
                        .spacing(8),
                )
                .spacing(8)
                .padding(12)
                .into();
        }
        let search = &self.collection.search;
        let genres = self
            .collection
            .browser
            .genres(&self.collection.songs, search);
        let artists = self
            .collection
            .browser
            .artists(&self.collection.songs, search);
        let albums = self
            .collection
            .browser
            .albums(&self.collection.songs, search);
        let tracks = self.collection.visible_tracks();
        let cursor = self.player.current().map(|t| t.url.as_str());
        widget::column::with_capacity(4)
            .push(
                widget::row::with_capacity(3)
                    .push(
                        widget::text_input::search_input("Search library", search)
                            .on_input(Message::SearchChanged),
                    )
                    .push(widget::button::standard("Import folder").on_press(Message::AddFolder))
                    .spacing(8),
            )
            .push(
                widget::row::with_capacity(3)
                    .push(Self::browser_column(
                        "Genre",
                        self.collection.browser.genre.clone(),
                        genres,
                        Message::SelectGenre(None),
                        |name| Message::SelectGenre(Some(name)),
                    ))
                    .push(Self::browser_column(
                        "Artist",
                        self.collection.browser.artist.clone(),
                        artists,
                        Message::SelectArtist(None),
                        |name| Message::SelectArtist(Some(name)),
                    ))
                    .push(Self::browser_column(
                        "Album",
                        self.collection.browser.album.clone(),
                        albums,
                        Message::SelectAlbum(None),
                        |name| Message::SelectAlbum(Some(name)),
                    ))
                    .height(Length::FillPortion(2)),
            )
            .push(
                widget::container(Self::song_table(
                    tracks,
                    cursor.map(str::to_string),
                    self.selected_index,
                    Message::PlayVisibleIndex,
                    Message::SelectVisibleIndex,
                    "+",
                    Message::EnqueueVisibleIndex,
                ))
                .height(Length::FillPortion(3)),
            )
            .spacing(8)
            .padding(8)
            .height(Length::Fill)
            .into()
    }

    fn queue_page(&self) -> Element<'_, Message> {
        let songs: Vec<Song> = self
            .player
            .queue()
            .iter()
            .map(|track| Song {
                title: track.title.clone(),
                artist: track.artist.clone(),
                album: track.album.clone(),
                url: track.url.clone(),
                length_ns: track.length_ns,
                track: track.track,
                year: track.year,
                genre: track.genre.clone(),
                ..Song::default()
            })
            .collect();
        let cursor = self.player.current().map(|t| t.url.as_str());
        widget::column::with_capacity(3)
            .push(
                widget::row::with_capacity(2)
                    .push(widget::text::heading("Play Queue").width(Length::Fill))
                    .push(widget::button::text("Clear").on_press(Message::ClearQueue)),
            )
            .push(Self::song_table(
                songs,
                cursor.map(str::to_string),
                self.selected_index,
                Message::PlayQueueIndex,
                Message::SelectVisibleIndex,
                "−",
                Message::RemoveQueueIndex,
            ))
            .spacing(8)
            .padding(8)
            .height(Length::Fill)
            .into()
    }

    fn playlists_page(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(8)
            .push(widget::text::heading("Playlists"))
            .push(widget::text::caption(
                "Smart playlists fill the Play Queue from the library. Saved playlists load from the Orange database.",
            ))
            .push(
                widget::row::with_capacity(3)
                    .push(widget::button::text("Never played").on_press(Message::SmartNeverPlayed))
                    .push(widget::button::text("Highest rated").on_press(Message::SmartHighestRated))
                    .push(widget::button::text("Most played").on_press(Message::SmartMostPlayed))
                    .spacing(8),
            );
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
        widget::scrollable(col.spacing(8).padding(8)).into()
    }

    fn radio_page(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(8)
            .push(widget::text::heading("Radio"))
            .push(widget::text::caption(
                "Click a station to play it. Custom streams need an http(s) URL.",
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
        widget::scrollable(col.spacing(8).padding(8)).into()
    }

    fn files_page(&self) -> Element<'_, Message> {
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
        col.spacing(8).padding(8).into()
    }

    fn devices_page(&self) -> Element<'_, Message> {
        let mut col = widget::column::with_capacity(6)
            .push(widget::text::heading("Devices"))
            .push(widget::text::caption(
                "USB mass-storage, MTP, and iPod Classic. Connect a player to copy music from the Play Queue.",
            ));
        for family in [
            DeviceFamily::UsbMassStorage,
            DeviceFamily::Mtp,
            DeviceFamily::IPod,
        ] {
            col = col.push(widget::text(format!("• {} — supported", family.name())));
        }
        col.spacing(8).padding(8).into()
    }

    fn settings_page(&self) -> Element<'_, Message> {
        let mode_button = |mode: AppearanceMode, label: &'static str| {
            widget::button::text(label).on_press(Message::AppearanceSelected(mode))
        };
        let mut folders = widget::column::with_capacity(self.collection.directories.len() + 2)
            .push(widget::text::heading("Music folders"));
        if self.collection.directories.is_empty() {
            folders = folders.push(widget::text::caption("No folders imported yet."));
        }
        for dir in &self.collection.directories {
            folders = folders.push(
                widget::row::with_capacity(2)
                    .push(widget::text(&dir.path).width(Length::Fill))
                    .push(
                        widget::button::destructive("Remove")
                            .on_press(Message::RemoveFolder(dir.id)),
                    )
                    .spacing(8),
            );
        }
        folders = folders.push(
            widget::row::with_capacity(3)
                .push(
                    widget::text_input("Folder path", &self.collection.add_path)
                        .on_input(Message::AddPathChanged),
                )
                .push(widget::button::suggested("Add").on_press(Message::AddFolder))
                .push(widget::button::standard("Rescan").on_press(Message::Rescan))
                .spacing(8),
        );
        let mut eq = widget::column::with_capacity(Equalizer::BANDS + 1)
            .push(widget::text::heading("Equalizer"));
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
        widget::scrollable(
            widget::column::with_capacity(8)
                .push(folders.spacing(8))
                .push(widget::text::heading("Appearance"))
                .push(
                    widget::row::with_capacity(3)
                        .push(mode_button(AppearanceMode::System, "System"))
                        .push(mode_button(AppearanceMode::Light, "Light"))
                        .push(mode_button(AppearanceMode::Dark, "Dark"))
                        .spacing(8),
                )
                .push(eq.spacing(4))
                .push(widget::text::heading("About"))
                .push(widget::text(about::body()))
                .spacing(12)
                .padding(16),
        )
        .into()
    }

    fn lyrics_body(&self) -> Element<'_, Message> {
        let (title, artist, album, lyrics) = match self.player.current() {
            Some(track) => {
                let lyrics = self
                    .collection
                    .song_by_url(&track.url)
                    .map(|s| s.lyrics.clone())
                    .filter(|l| !l.trim().is_empty())
                    .unwrap_or_else(|| String::from("No lyrics stored for this track."));
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
        widget::column::with_capacity(6)
            .push(widget::text(title))
            .push(widget::text(artist))
            .push(widget::text(album))
            .push(widget::text(lyrics))
            .spacing(8)
            .into()
    }
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
        let mut core = core;
        core.window.use_template = false;
        core.window.show_headerbar = false;
        core.window.content_container = false;
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
            page: Page::Library,
            selected_index: None,
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
            show_lyrics: false,
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
            app.status = format!("Library: {err}");
        } else if app.collection.songs.is_empty() {
            app.status = String::from("Import a music folder to build the library.");
        } else {
            app.status = track_list_summary(&app.collection.songs);
        }
        let title_task = match app.core.main_window_id() {
            Some(id) => app.set_window_title(String::from("Orange Music Player"), id),
            None => cosmic::app::Task::none(),
        };
        (app, title_task)
    }

    fn subscription(&self) -> cosmic::iced::Subscription<Self::Message> {
        if self.player.state() == EngineState::Playing {
            cosmic::iced::time::every(Duration::from_millis(100)).map(|_| Message::Tick)
        } else {
            cosmic::iced::Subscription::none()
        }
    }

    fn update(&mut self, message: Self::Message) -> cosmic::app::Task<Self::Message> {
        #[cfg(feature = "dbus")]
        self.poll_remotes();
        match message {
            Message::PlayPause => self.play_or_resume(),
            Message::Stop => {
                self.player.stop();
                self.position_secs = 0;
                self.sync_engine();
                self.status = String::from("Stopped.");
            }
            Message::Next => self.skip_next(),
            Message::Previous => {
                if !self.player.previous() {
                    if let Some(cursor) = self.player.cursor() {
                        self.player.play_at(cursor);
                    }
                }
                self.sync_engine();
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
                #[cfg(feature = "gst")]
                if let Some(engine) = self.engine.as_ref() {
                    let _ = engine.set_output_volume(self.volume as f64 / 100.0);
                }
            }
            Message::AppearanceSelected(mode) => {
                self.appearance = mode;
                return self.apply_appearance();
            }
            Message::SearchChanged(search) => {
                self.collection.search = search;
                self.selected_index = None;
            }
            Message::AddPathChanged(path) => self.collection.add_path = path,
            Message::AddFolder => {
                let path = self.collection.add_path.clone();
                match self.collection.add_folder(&path) {
                    Ok(report) => {
                        self.status =
                            format!("Imported {} songs from {}.", report.songs, report.path);
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
                    self.status = format!("Rescan complete: {} songs.", report.songs);
                }
                Err(e) => self.status = e,
            },
            Message::SelectGenre(genre) => {
                self.collection.browser.select_genre(genre);
                self.selected_index = None;
            }
            Message::SelectArtist(artist) => {
                self.collection.browser.select_artist(artist);
                self.selected_index = None;
            }
            Message::SelectAlbum(album) => {
                self.collection.browser.select_album(album);
                self.selected_index = None;
            }
            Message::SelectPage(page) => {
                self.page = page;
                self.selected_index = None;
            }
            Message::SelectVisibleIndex(index) => self.selected_index = Some(index),
            Message::PlayVisibleIndex(index) => {
                let tracks = self.collection.visible_tracks();
                self.replace_songs_at(tracks, index);
            }
            Message::EnqueueVisibleIndex(index) => {
                let tracks = self.collection.visible_tracks();
                if let Some(song) = tracks.get(index).cloned() {
                    self.enqueue_songs(vec![song], self.player.state() != EngineState::Playing);
                }
            }
            Message::PlayQueueIndex(index) => {
                if self.player.play_at(index) {
                    self.sync_engine();
                }
            }
            Message::ClearQueue => {
                self.player.clear_queue();
                self.sync_engine();
                self.status = String::from("Play Queue cleared.");
            }
            Message::RemoveQueueIndex(index) => {
                self.player.remove_at(index);
            }
            Message::PlayRadio { name, url } => {
                self.replace_songs_at(
                    vec![Song {
                        title: name.clone(),
                        artist: String::from("Radio"),
                        url,
                        ..Song::default()
                    }],
                    0,
                );
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
                self.replace_songs_at(vec![song], 0);
            }
            Message::FilesEnqueue(path) => {
                let song = orange_collection::scan::song_from_path(&path, &self.files.cwd, -1);
                self.enqueue_songs(vec![song], false);
            }
            Message::FilesPlayCwd => {
                self.replace_songs_at(self.files.audio_in_cwd(), 0);
            }
            Message::LoadSavedPlaylist(id) => match self.collection.load_saved_playlist(id) {
                Ok(songs) => {
                    self.page = Page::Queue;
                    self.replace_songs_at(songs, 0);
                }
                Err(e) => self.status = e,
            },
            Message::SmartNeverPlayed => {
                self.page = Page::Queue;
                self.replace_songs_at(smart_never_played(&self.collection.songs), 0);
            }
            Message::SmartHighestRated => {
                self.page = Page::Queue;
                self.replace_songs_at(smart_highest_rated(&self.collection.songs), 0);
            }
            Message::SmartMostPlayed => {
                self.page = Page::Queue;
                self.replace_songs_at(smart_most_played(&self.collection.songs), 0);
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
            Message::ToggleLyrics => {
                self.show_lyrics = !self.show_lyrics;
            }
            Message::EqBand { band, db } => {
                self.equalizer.set_gain(band, db as f64);
                #[cfg(feature = "gst")]
                {
                    self.engine_url.clear();
                }
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
        let page = match self.active_page() {
            Page::Library => self.library_page(),
            Page::Queue => self.queue_page(),
            Page::Playlists => self.playlists_page(),
            Page::Radio => self.radio_page(),
            Page::Files => self.files_page(),
            Page::Devices => self.devices_page(),
            Page::Settings => self.settings_page(),
        };
        let mut body = widget::row::with_capacity(4)
            .push(self.source_list())
            .push(widget::divider::vertical::default())
            .push(
                widget::container(page)
                    .width(Length::Fill)
                    .height(Length::Fill),
            );
        if self.show_lyrics {
            body = body.push(widget::divider::vertical::default()).push(
                widget::container(widget::scrollable(self.lyrics_body()).height(Length::Fill))
                    .width(Length::Fixed(260.0))
                    .height(Length::Fill)
                    .padding(8),
            );
        }
        widget::container(
            widget::column::with_capacity(4)
                .push(self.player_strip())
                .push(widget::divider::horizontal::default())
                .push(body.height(Length::Fill))
                .push(
                    widget::container(widget::text::caption(self.status_text()))
                        .padding([4, 8])
                        .width(Length::Fill),
                )
                .width(Length::Fill)
                .height(Length::Fill),
        )
        .class(cosmic::theme::Container::Background)
        .width(Length::Fill)
        .height(Length::Fill)
        .into()
    }
}
