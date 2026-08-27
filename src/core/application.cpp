#include "core/application.h"

#include "constants/collectionsettings.h"
#include "constants/notificationssettings.h"
#include "core/appearance.h"
#include "equalizer/equalizerpersist.h"
#include "playlist/playlist.h"
#include "playlist/playlistqueuescope.h"
#include "core/logging.h"
#include "core/settings.h"
#include "collection/skipcounteligibility.h"
#include "scrobbler/scrobblereligibility.h"
#include "scrobbler/scrobblerlifecycle.h"
#include "core/exitfade.h"
#include "constants/behavioursettings.h"
#include "core/commandlineurlplan.h"
#include "core/commandlinevolume.h"
#include "tidal/tidalloginurl.h"
#include "tidal/tidalservice.h"
#include "utilities/fileutils.h"

#include "config.h"
#ifdef HAVE_SPOTIFY
#include "spotify/spotifyservice.h"
#endif

Application::Application()
    : task_manager_(std::make_unique<TaskManager>()),
      database_(std::make_unique<Database>()),
      network_(std::make_unique<NetworkAccessManager>()),
      tagreader_(std::make_unique<TagReader>()),
      tagreader_client_(std::make_unique<TagReaderClient>(tagreader_.get())),
      url_handlers_(std::make_unique<UrlHandlers>()),
      collection_(std::make_unique<CollectionLibrary>(database_.get(), task_manager_.get(), tagreader_.get())),
      playlist_backend_(std::make_unique<PlaylistBackend>(database_.get(), tagreader_.get(), collection_->backend())),
      playlist_manager_(std::make_unique<PlaylistManager>(task_manager_.get(), tagreader_.get(), url_handlers_.get(),
                                                          playlist_backend_.get(), collection_->backend())),
      player_(std::make_unique<Player>(task_manager_.get(), url_handlers_.get(), playlist_manager_.get())),
      queue_(std::make_unique<Queue>()),
      cover_providers_(std::make_unique<CoverProviders>(network_.get())),
      albumcover_loader_(std::make_unique<AlbumCoverLoader>(tagreader_.get())),
      current_albumcover_loader_(std::make_unique<CurrentAlbumCoverLoader>(albumcover_loader_.get())),
      lyrics_providers_(std::make_unique<LyricsProviders>(network_.get())),
      lyrics_fetcher_(std::make_unique<LyricsFetcher>(lyrics_providers_.get())),
      scrobbler_(std::make_unique<AudioScrobbler>(network_.get())),
      streaming_services_(std::make_unique<StreamingServices>(network_.get(), url_handlers_.get(), task_manager_.get())),
      radio_services_(std::make_unique<RadioServices>(database_.get(), network_.get())),
      device_manager_(std::make_unique<DeviceManager>(database_.get(), task_manager_.get())),
      device_finders_(std::make_unique<DeviceFinders>()),
      equalizer_(std::make_unique<Equalizer>()),
      analyzer_(std::make_unique<Analyzer>()),
      moodbar_loader_(std::make_unique<MoodbarLoader>()),
      moodbar_(std::make_unique<MoodbarController>(moodbar_loader_.get())),
      waveform_loader_(std::make_unique<WaveformLoader>()),
      waveform_(std::make_unique<WaveformController>(waveform_loader_.get())),
      transcoder_(std::make_unique<Transcoder>()),
      osd_(std::make_unique<OSD>()),
      tray_(std::make_unique<SystemTrayIcon>()),
      shortcuts_(std::make_unique<GlobalShortcutsManager>()),
      discord_(std::make_unique<DiscordRichPresence>()),
      tag_fetcher_(std::make_unique<TagFetcher>(network_.get())) {}

Application::~Application() {
  if (!exit_started_) {
    CompleteExit();
  }
}

void Application::Init() {
  Appearance appearance;
  appearance.Apply();
  database_->Open();
  collection_->Init();
  {
    Settings settings;
    settings.BeginGroup("Collection");
    if (settings.BoolValue("startup_scan", settings.BoolValue("startupscan", true))) {
      collection_->IncrementalScan();
    }
  }
  playlist_manager_->Init();
  playlist_manager_->set_tagreader_client(tagreader_client_.get());
  playlist_manager_->PlayRequested.Connect([this](int row) {
    if (player_) {
      player_->PlayAt(row);
    }
  });
  player_->Init();
#ifdef HAVE_SPOTIFY
  if (auto *spotify = dynamic_cast<SpotifyService *>(streaming_services_->ServiceByName("Spotify"))) {
    auto push_token = [this, spotify]() { player_->engine()->UpdateSpotifyAccessToken(spotify->access_token()); };
    spotify->AuthenticationChanged.Connect(push_token);
    push_token();
  }
#endif
  BindPlayerQueue();
  playlist_manager_->CurrentChanged.Connect([this](Playlist *) { BindPlayerQueue(); });
  playlist_manager_->ActiveChanged.Connect([this](Playlist *) { BindPlayerQueue(); });
  playlist_manager_->PlaylistsLoaded.Connect([this]() { BindPlayerQueue(); });
  device_finders_->Init();
  device_manager_->set_tagreader(tagreader_.get());
  device_manager_->set_network(network_.get());
  device_manager_->Init();
  url_handlers_->AddHandler(device_manager_->url_handler());
  osd_->set_tray_icon(tray_.get());
  osd_->ReloadSettings();
  shortcuts_->Init();
  discord_->ReloadSettings();
  tray_->SetupStatusNotifier();
  tray_->PlayPause.Connect([this]() { player_->PlayPause(); });
  tray_->Stop.Connect([this]() { player_->Stop(); });
  tray_->Next.Connect([this]() { player_->Next(); });
  tray_->Previous.Connect([this]() { player_->Previous(); });
  tray_->Mute.Connect([this]() { player_->Mute(); });
  tray_->StopAfter.Connect([this]() {
    player_->StopAfterCurrent();
    osd_->StopAfterToggle(player_->stop_after_current());
  });
  tray_->Love.Connect([this]() { scrobbler_->Love(player_->current_song()); });
  tray_->Quit.Connect([this]() { Exit(); });
  player_->engine()->SetEqualizerEnabled(equalizer_->enabled());
  player_->engine()->SetEqualizerParameters(equalizer_->EffectivePreamp(), equalizer_->EffectiveGains());
  player_->engine()->SetStereoBalance(equalizer_->EffectiveBalanceFraction());
  tray_->VolumeScroll.Connect([this](int delta) {
    if (delta > 0) {
      player_->VolumeUp();
    } else if (delta < 0) {
      player_->VolumeDown();
    }
  });
  mpris_ = std::make_unique<Mpris2>(this);

  player_->SongChanged.Connect([this](const Song &song) {
    collection_->CurrentSongChanged(song);
    scrobbler_->NowPlaying(song);
    current_albumcover_loader_->Load(song);
    if (Playlist *playlist = playlist_manager_->active()) {
      playlist->ApplyDiscoveredArt(song, albumcover_loader_->LoadPath(song));
    }
    osd_->SongChanged(song, current_albumcover_loader_->current());
    moodbar_->CurrentSongChanged(song);
    waveform_->CurrentSongChanged(song);
    discord_->UpdatePresence(song, player_->GetState() == GstEngine::State::Playing);
    tray_->SetNowPlaying(song);
  });
  player_->NowPlayingRefresh.Connect([this](const Song &song) { scrobbler_->NowPlaying(song); });
  current_albumcover_loader_->AlbumCoverReady.Connect([this](const Song &song, const std::vector<unsigned char> &art) {
    osd_->AlbumCoverLoaded(song, art);
  });
  player_->TrackSkipped.Connect([this](const Song &song, int64_t pos_ns, int64_t len_ns) {
    if (song.id() > 0 && SkipCountEligibility::ShouldIncrement(pos_ns, len_ns)) {
      collection_->backend()->IncrementSkipCount(song.id());
    }
  });
  player_->TrackEndedPlaycount.Connect([this](const Song &song) {
    Song tagged = song;
    tagged.set_playcount(song.playcount() + 1);
    collection_->SongsPlaycountChanged({tagged});
  });
  player_->PlaybackFinished.Connect([this](const Song &song, int64_t listened_nanosec) {
    Playlist *playlist = playlist_manager_->active();
    const bool already_scrobbled = playlist && playlist->scrobbled();
    if (!already_scrobbled && ScrobblerEligibility::ShouldScrobble(song, listened_nanosec)) {
      scrobbler_->Scrobble(song);
    }
  });
  player_->ForceShowOSD.Connect([this](const Song &song, bool toggle) {
    if (toggle) {
      osd_->SetPrettyOSDToggleMode(true);
    }
    osd_->ReshowCurrentSong(song, current_albumcover_loader_->current());
  });
  player_->Playing.Connect([this]() {
    tray_->SetPlaying(true);
    if (playback_was_paused_) {
      osd_->Resumed();
    }
    playback_was_paused_ = false;
    discord_->UpdatePresence(player_->current_song(), true);
  });
  player_->Paused.Connect([this]() {
    playback_was_paused_ = true;
    tray_->SetPaused();
    osd_->Paused();
    discord_->Clear();
  });
  player_->PositionChanged.Connect([this](int64_t position_nanosec, int64_t) {
    if (player_->GetState() == GstEngine::State::Playing) {
      discord_->RefreshAfterSeek(player_->current_song(), position_nanosec / 1000000000LL);
    }
  });
  player_->Stopped.Connect([this]() {
    playback_was_paused_ = false;
    collection_->Stopped();
    scrobbler_->ClearPlaying();
    moodbar_->PlaybackStopped();
    waveform_->PlaybackStopped();
    discord_->Clear();
    tray_->SetStopped();
    tray_->ClearNowPlaying();
    osd_->Stopped();
  });
  player_->VolumeChanged.Connect([this](unsigned volume) { osd_->VolumeChanged(volume); });
  player_->PlaylistFinished.Connect([this]() { osd_->PlaylistFinished(); });
  WatchPlaylistOsd(playlist_manager_->current());
  playlist_manager_->CurrentChanged.Connect([this](Playlist *playlist) { WatchPlaylistOsd(playlist); });
  playlist_manager_->PlaylistsLoaded.Connect([this]() { WatchPlaylistOsd(playlist_manager_->current()); });
  equalizer_->ParametersChanged.Connect([this](bool enabled, int preamp, const std::vector<int> &gains) {
    player_->engine()->SetEqualizerEnabled(enabled);
    player_->engine()->SetEqualizerParameters(EqualizerPersist::EffectivePreamp(enabled, preamp),
                                              EqualizerPersist::EffectiveGains(enabled, gains));
  });
  equalizer_->StereoBalanceChanged.Connect([this](float balance) { player_->engine()->SetStereoBalance(balance); });
  shortcuts_->Play.Connect([this]() { player_->Play(); });
  shortcuts_->Pause.Connect([this]() { player_->Pause(); });
  shortcuts_->PlayPause.Connect([this]() { player_->PlayPause(); });
  shortcuts_->Stop.Connect([this]() { player_->Stop(); });
  shortcuts_->StopAfter.Connect([this]() {
    player_->StopAfterCurrent();
    osd_->StopAfterToggle(player_->stop_after_current());
  });
  shortcuts_->Next.Connect([this]() { player_->Next(); });
  shortcuts_->Previous.Connect([this]() { player_->Previous(); });
  shortcuts_->RestartOrPrevious.Connect([this]() { player_->RestartOrPrevious(); });
  shortcuts_->VolumeUp.Connect([this]() { player_->VolumeUp(); });
  shortcuts_->VolumeDown.Connect([this]() { player_->VolumeDown(); });
  shortcuts_->Mute.Connect([this]() { player_->Mute(); });
  shortcuts_->SeekForward.Connect([this]() { player_->SeekForward(); });
  shortcuts_->SeekBackward.Connect([this]() { player_->SeekBackward(); });
  shortcuts_->ShowHide.Connect([this]() { RaiseRequested.Emit(); });
  shortcuts_->ShowOSD.Connect([this]() { player_->ShowOSD(); });
  shortcuts_->TogglePrettyOSD.Connect([this]() { player_->TogglePrettyOSD(); });
  shortcuts_->CycleShuffle.Connect([this]() { playlist_manager_->CycleShuffleMode(); });
  shortcuts_->CycleRepeat.Connect([this]() { playlist_manager_->CycleRepeatMode(); });
  shortcuts_->ToggleScrobbling.Connect([this]() { scrobbler_->ToggleScrobbling(); });
  shortcuts_->Love.Connect([this]() { scrobbler_->Love(player_->current_song()); });
}

void Application::Exit() {
  ++exit_count_;
  const bool playing = player_ && player_->GetState() == EngineBase::State::Playing;
  const bool fadeout_enabled = player_ && player_->engine() && player_->engine()->fading_enabled();
  const ExitFade::Action action = ExitFade::Decide(exit_count_, fadeout_enabled, playing, exit_started_);
  if (action == ExitFade::Action::WaitForFade) {
    waiting_for_fade_ = true;
    HideForExit.Emit();
    if (player_ && player_->engine()) {
      player_->engine()->Finished.Connect([this]() { CompleteExit(); });
    }
    if (player_) {
      player_->SaveVolume();
      player_->SavePlaybackStatus();
      player_->Stop();
    }
    return;
  }
  if (action == ExitFade::Action::AbortProcess && exit_started_) {
    return;
  }
  CompleteExit();
}

void Application::CompleteExit() {
  if (exit_started_) {
    return;
  }
  exit_started_ = true;
  waiting_for_fade_ = false;
  if (player_) {
    player_->SaveVolume();
    player_->SavePlaybackStatus();
    player_->Stop();
  }
  if (playlist_manager_) {
    playlist_manager_->SaveActive();
  }
  if (scrobbler_ && ScrobblerLifecycle::ShouldFlushOnExit(scrobbler_->enabled())) {
    scrobbler_->WriteCache();
  }
  if (database_) {
    database_->Backup();
    database_->Close();
  }
  ExitFinished.Emit();
}

void Application::ApplyCommandline(const CommandlineOptions &options) {
  if (TidalLoginUrl::ConsumesCommandline(options.urls())) {
    if (auto *tidal = dynamic_cast<TidalService *>(streaming_services_->ServiceByName("Tidal"))) {
      tidal->AuthorizationUrlReceived(TidalLoginUrl::Find(options.urls()));
    }
    return;
  }
  if (!options.urls().empty()) {
    Settings settings;
    settings.BeginGroup(BehaviourSettings::kSettingsGroup);
    const auto add_mode = static_cast<BehaviourSettings::AddBehaviour>(
        settings.IntValue(BehaviourSettings::kDoubleClickAddMode, static_cast<int>(BehaviourSettings::kDefaultDoubleClickAddMode)));
    const auto play_mode = static_cast<BehaviourSettings::PlayBehaviour>(
        settings.IntValue(BehaviourSettings::kDoubleClickPlayMode, static_cast<int>(BehaviourSettings::kDefaultDoubleClickPlayMode)));
    const bool playing = player_ && player_->GetState() == GstEngine::State::Playing;
    const CollectionBehaviour::Plan plan =
        CommandlineUrlPlan::FromOptions(options.url_list_action(), options.player_action(), add_mode, play_mode, playing);
    if (plan.destination == CollectionBehaviour::Destination::New) {
      playlist_manager_->New(CommandlineUrlPlan::NewPlaylistName(options.playlist_name()));
    } else if (plan.clear_current) {
      playlist_manager_->ClearCurrent();
    }
    playlist_manager_->InsertUrls(options.urls(), -1, plan.should_play, plan.queue == CollectionBehaviour::QueueMode::Append,
                                 plan.queue == CollectionBehaviour::QueueMode::Next);
  }
  switch (options.player_action()) {
    case CommandlineOptions::PlayerAction::Play:
      if (!CommandlineUrlPlan::SkipStandalonePlay(!options.urls().empty(), options.player_action())) {
        player_->Play();
      }
      break;
    case CommandlineOptions::PlayerAction::PlayPause:
      player_->PlayPause();
      break;
    case CommandlineOptions::PlayerAction::Pause:
      player_->Pause();
      break;
    case CommandlineOptions::PlayerAction::Stop:
      player_->Stop();
      break;
    case CommandlineOptions::PlayerAction::Previous:
      player_->Previous();
      break;
    case CommandlineOptions::PlayerAction::Next:
      player_->Next();
      break;
    case CommandlineOptions::PlayerAction::RestartOrPrevious:
      player_->RestartOrPrevious();
      break;
    case CommandlineOptions::PlayerAction::StopAfterCurrent:
      player_->StopAfterCurrent();
      break;
    case CommandlineOptions::PlayerAction::PlayPlaylist:
      player_->PlayPlaylist(options.playlist_name());
      break;
    case CommandlineOptions::PlayerAction::ResizeWindow:
      RaiseRequested.Emit();
      break;
    default:
      break;
  }
  if (!options.log_levels().empty()) {
    logging::SetLevels(options.log_levels());
  }
  if (options.set_volume() >= 0) {
    player_->SetVolume(static_cast<unsigned>(options.set_volume()));
  }
  if (options.volume_modifier() != 0) {
    player_->SetVolume(static_cast<unsigned>(CommandlineVolume::Apply(static_cast<int>(player_->GetVolume()), options.volume_modifier())));
  }
  if (options.seek_to() >= 0) {
    player_->SeekTo(options.seek_to());
  }
  if (options.seek_by() != 0) {
    player_->SeekTo(player_->engine()->position_nanosec() / 1000000000LL + options.seek_by());
  }
  if (options.play_track_at() >= 0) {
    player_->PlayAt(options.play_track_at());
  }
  if (options.show_osd()) {
    player_->ShowOSD();
  }
  if (options.toggle_pretty_osd()) {
    player_->TogglePrettyOSD();
  }
}

Queue *Application::queue() const { return PlaylistQueueScope::For(playlist_manager_ ? playlist_manager_->current() : nullptr, queue_.get()); }

void Application::BindPlayerQueue() {
  if (player_) {
    player_->SetQueue(PlaylistQueueScope::For(playlist_manager_ ? playlist_manager_->active() : nullptr, queue_.get()));
  }
}

void Application::WatchPlaylistOsd(Playlist *playlist) {
  ++playlist_osd_gen_;
  const int gen = playlist_osd_gen_;
  if (!playlist) {
    return;
  }
  playlist->RepeatModeChanged.Connect([this, gen]() {
    if (gen != playlist_osd_gen_ || !playlist_manager_->current()) {
      return;
    }
    osd_->RepeatModeChanged(playlist_manager_->current()->repeat_mode());
  });
  playlist->ShuffleModeChanged.Connect([this, gen]() {
    if (gen != playlist_osd_gen_ || !playlist_manager_->current()) {
      return;
    }
    osd_->ShuffleModeChanged(playlist_manager_->current()->shuffle_mode());
  });
}
