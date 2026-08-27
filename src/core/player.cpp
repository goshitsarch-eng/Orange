#include "core/player.h"

#include "core/playeritemoptions.h"
#include "collection/collectionbackend.h"
#include "collection/playcountincrement.h"
#include "playlist/playlistplayrow.h"

#include <algorithm>
#include <ctime>
#include <memory>

#include <glib.h>

#include "constants/backendsettings.h"
#include "constants/behavioursettings.h"
#include "constants/playlistsettings.h"
#include "core/logging.h"
#include "core/playermetadatasync.h"
#include "core/playernextmetadata.h"
#include "core/playererrorloop.h"
#include "core/playerloadresult.h"
#include "core/playerprevious.h"
#include "core/playerseeknotify.h"
#include "core/playerstreamexpire.h"
#include "core/playerintro.h"
#include "core/playerpreload.h"
#include "core/playerrepeat.h"
#include "core/playerstopafter.h"
#include "core/songsegment.h"
#include "core/playerresume.h"
#include "core/playlistsloadedgate.h"
#include "core/settings.h"
#include "core/urlhandlers.h"
#include "playlist/playlist.h"
#include "playlist/playlistbehaviour.h"
#include "playlist/playlistmanager.h"
#include "playlist/playlistsequence.h"
#include "queue/queue.h"
#include "queue/queuerows.h"

Player::Player(TaskManager *task_manager, UrlHandlers *url_handlers, PlaylistManager *playlist_manager)
    : task_manager_(task_manager), url_handlers_(url_handlers), playlist_manager_(playlist_manager), engine_(std::make_unique<GstEngine>()) {}

Player::~Player() { CancelIntroTimeout(); }

void Player::Init() {
  engine_->SetTaskManager(task_manager_);
  engine_->Init();
  engine_->StateChanged.Connect([this](EngineBase::State state) { HandleEngineState(state); });
  engine_->TrackEnded.Connect([this]() { HandleTrackEnded(); });
  engine_->TrackAboutToEnd.Connect([this]() { PreloadNext(); });
  engine_->Error.Connect([this](const std::string &error) { HandleEngineError(error); });
  engine_->MetadataReceived.Connect([this](const Song &song) { HandleEngineMetadata(song); });
  ReloadSettings();
  LoadVolume();
}

EngineBase::State Player::GetState() const { return engine_->state(); }

void Player::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  continue_on_error_ = settings.BoolValue(PlaylistSettings::kContinueOnError, PlaylistSettings::kDefaultContinueOnError);
  greyout_ = settings.BoolValue(PlaylistSettings::kGreyoutSongsPlay, PlaylistSettings::kDefaultGreyoutSongsPlay);
  settings.EndGroup();
  settings.BeginGroup("Behaviour");
  seek_step_sec_ = settings.Contains("seek_step_sec") ? settings.IntValue("seek_step_sec", 10) : settings.IntValue("seekstep", 10);
  volume_increment_ = static_cast<unsigned>(settings.Contains("volume_increment") ? settings.IntValue("volume_increment", 5)
                                                                                : settings.IntValue("volumeincrement", 5));
  menu_previous_mode_ = static_cast<BehaviourSettings::PreviousBehaviour>(
      settings.IntValue(BehaviourSettings::kMenuPreviousMode, static_cast<int>(BehaviourSettings::kDefaultMenuPreviousMode)));
  settings.EndGroup();
  settings.BeginGroup("Backend");
  const bool fading = settings.Contains("FadeoutEnabled") ? settings.BoolValue("FadeoutEnabled") : settings.BoolValue("fading", false);
  engine_->SetFadingEnabled(fading);
  engine_->SetAutoCrossfadeEnabled(settings.Contains("AutoCrossfadeEnabled") ? settings.BoolValue("AutoCrossfadeEnabled")
                                                                            : settings.BoolValue("autocrossfade", fading));
  engine_->SetFadeDurationMs(settings.Contains("FadeoutDuration") ? settings.IntValue("FadeoutDuration", 2000)
                                                                 : settings.IntValue("fadeduration", 2000));
  engine_->SetPlaybin3(settings.BoolValue(BackendSettings::kPlaybin3, BackendSettings::kDefaultPlaybin3));
  engine_->SetNoCrossfadeSameAlbum(settings.BoolValue(BackendSettings::kNoCrossfadeSameAlbum, BackendSettings::kDefaultNoCrossfadeSameAlbum));
  engine_->SetFadeoutPauseEnabled(settings.BoolValue(BackendSettings::kFadeoutPauseEnabled, BackendSettings::kDefaultFadeoutPauseEnabled));
  engine_->SetFadeoutPauseDurationMs(
      settings.IntValue(BackendSettings::kFadeoutPauseDuration, static_cast<int>(BackendSettings::kDefaultFadeoutPauseDuration)));
  engine_->SetReplayGainEnabled(settings.BoolValue(BackendSettings::kRgEnabled, BackendSettings::kDefaultRgEnabled));
  engine_->SetReplayGainMode(settings.IntValue(BackendSettings::kRgMode, BackendSettings::kDefaultRgMode));
  engine_->SetReplayGainPreamp(settings.DoubleValue(BackendSettings::kRgPreamp, BackendSettings::kDefaultRgPreamp));
  engine_->SetOutput(settings.Contains("output") ? settings.Value("output", "autoaudiosink") : settings.Value("Output", "autoaudiosink"),
                     settings.Contains("device") ? settings.Value("device") : settings.Value("Device"));
  engine_->ReloadBackendOptions();
}

void Player::LoadVolume() {
  Settings settings;
  settings.BeginGroup("Player");
  SetVolume(static_cast<unsigned>(settings.IntValue("volume", 100)));
}

void Player::SaveVolume() {
  Settings settings;
  settings.BeginGroup("Player");
  settings.SetIntValue("volume", static_cast<int>(volume_));
  settings.Sync();
}

void Player::SavePlaybackStatus() {
  Settings settings;
  settings.BeginGroup(PlayerResume::kSettingsGroup);
  const int state = static_cast<int>(GetState());
  settings.SetIntValue(PlayerResume::kPlaybackState, state);
  if (PlayerResume::IsResumableState(state) && playlist_manager_ && playlist_manager_->active()) {
    settings.SetIntValue(PlayerResume::kPlaybackPlaylist, playlist_manager_->active()->id());
    settings.SetIntValue(PlayerResume::kPlaybackPosition, static_cast<int>(engine_->position_nanosec() / 1000000000LL));
  } else {
    settings.SetIntValue(PlayerResume::kPlaybackPlaylist, -1);
    settings.SetIntValue(PlayerResume::kPlaybackPosition, 0);
  }
  settings.Sync();
}

void Player::PlaylistsLoaded() {
  playlists_loaded_ = true;
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  const bool resume = settings.BoolValue(BehaviourSettings::kResumePlayback, BehaviourSettings::kDefaultResumePlayback);
  settings.EndGroup();
  settings.BeginGroup(PlayerResume::kSettingsGroup);
  const int state = settings.IntValue(PlayerResume::kPlaybackState, static_cast<int>(EngineBase::State::Empty));
  if (PlaylistsLoadedGate::ShouldResumeAfterLoad(resume, state)) {
    ResumePlayback();
  } else if (PlaylistsLoadedGate::ShouldHonorPlayRequest(resume, state, play_requested_)) {
    Play();
  }
  play_requested_ = false;
}

void Player::ResumePlayback() {
  Settings settings;
  settings.BeginGroup(BehaviourSettings::kSettingsGroup);
  const bool enabled = settings.BoolValue(BehaviourSettings::kResumePlayback, BehaviourSettings::kDefaultResumePlayback);
  settings.EndGroup();
  settings.BeginGroup(PlayerResume::kSettingsGroup);
  const int state = settings.IntValue(PlayerResume::kPlaybackState, static_cast<int>(EngineBase::State::Empty));
  const int playlist_id = settings.IntValue(PlayerResume::kPlaybackPlaylist, -1);
  const int position_sec = settings.IntValue(PlayerResume::kPlaybackPosition, 0);
  settings.SetIntValue(PlayerResume::kPlaybackState, static_cast<int>(EngineBase::State::Empty));
  settings.SetIntValue(PlayerResume::kPlaybackPlaylist, -1);
  settings.SetIntValue(PlayerResume::kPlaybackPosition, 0);
  settings.Sync();
  if (!PlayerResume::ShouldResume(enabled, state) || !playlist_manager_ || playlist_id < 0 || !playlist_manager_->playlist(playlist_id)) {
    return;
  }
  playlist_manager_->SetCurrentPlaylist(playlist_id);
  playlist_manager_->SetActiveToCurrent();
  const int row = playlist_manager_->current_row();
  PlayAt(row < 0 ? 0 : row, PlayerResume::ShouldPause(state), static_cast<uint64_t>(PlayerResume::PositionToNanosec(position_sec)));
}

void Player::Play() {
  if (PlaylistsLoadedGate::DeferPlay(playlists_loaded_)) {
    play_requested_ = true;
    return;
  }
  if (playlist_manager_) {
    playlist_manager_->SetActiveToCurrent();
  }
  int row = 0;
  if (playlist_manager_ && playlist_manager_->active()) {
    Playlist *playlist = playlist_manager_->active();
    row = PlaylistPlayRow::Resolve(playlist->current_row(), playlist->last_played_row(), playlist->row_count());
  }
  PlayAt(row, false);
  if (playlist_manager_) {
    playlist_manager_->SetActivePlaying();
  }
}

void Player::PlayPause() {
  switch (engine_->state()) {
    case GstEngine::State::Playing:
      Pause();
      break;
    case GstEngine::State::Paused:
      UnPause();
      break;
    default:
      Play();
      break;
  }
}

void Player::Pause() {
  if (PlayerItemOptions::ShouldStopInsteadOfPause(current_song_)) {
    Stop();
    return;
  }
  engine_->Pause();
  if (playlist_manager_) {
    playlist_manager_->SetActivePaused();
  }
}

void Player::Stop(bool stop_after) {
  stop_after_current_ = stop_after;
  if (!stop_after) {
    CancelIntroTimeout();
    FinishCurrentPlayback();
    engine_->Stop();
    if (playlist_manager_) {
      playlist_manager_->SetActiveStopped();
    }
    Stopped.Emit();
  }
}

void Player::StopAfterCurrent() {
  stop_after_current_ = !stop_after_current_;
  if (playlist_manager_ && playlist_manager_->current()) {
    Playlist *playlist = playlist_manager_->current();
    if (stop_after_current_) {
      playlist->set_stop_after_row(playlist->current_row());
    } else {
      playlist->set_stop_after_row(-1);
    }
  }
}

int Player::SameAlbumFlags(int track_change_flags, const Song &from, const Song &to) const {
  if (from.IsOnSameAlbum(to)) {
    return track_change_flags | GstEngine::SameAlbum;
  }
  return track_change_flags;
}

void Player::MaybeEmitTrackSkipped() {
  if (finished_current_) {
    return;
  }
  if (!current_song_.is_valid() && current_song_.url().empty()) {
    return;
  }
  const int64_t pos = engine_ ? engine_->position_nanosec() : 0;
  const int64_t len = current_song_.length_nanosec();
  if (len > 0 && pos == len) {
    return;
  }
  TrackSkipped.Emit(current_song_, pos, len);
}

void Player::PlayQueueHead(int track_change_flags) {
  if (!queue_ || queue_->empty()) {
    return;
  }
  const Song previous = current_song_;
  const QueueRows::Source source = queue_->PeekSource();
  current_song_ = queue_->TakeNext();
  if (source.valid() && playlist_manager_) {
    if (Playlist *playlist = playlist_manager_->playlist(source.playlist_id)) {
      playlist_manager_->SetActivePlaylist(source.playlist_id);
      playlist->set_current_row(source.row);
      const Song from_playlist = playlist->song(source.row);
      if (from_playlist.is_valid() || !from_playlist.url().empty()) {
        current_song_ = from_playlist;
      }
    }
  }
  PlayLoadedSong(false, SameAlbumFlags(track_change_flags, previous, current_song_));
}

void Player::Next() { Advance(GstEngine::Manual); }

void Player::Advance(int track_change_flags) {
  if ((track_change_flags & GstEngine::Manual) != 0) {
    MaybeEmitTrackSkipped();
  }
  Song upcoming;
  if (queue_ && !queue_->empty()) {
    upcoming = queue_->songs().front();
  } else if (playlist_manager_) {
    upcoming = playlist_manager_->PeekNextSong();
  }
  track_change_flags = SameAlbumFlags(track_change_flags, current_song_, upcoming);
  FinishCurrentPlayback();
  if (queue_ && !queue_->empty()) {
    PlayQueueHead(track_change_flags);
    return;
  }
  if (!playlist_manager_) {
    return;
  }
  playlist_manager_->Next();
  PlayCurrent(false, 0, track_change_flags);
}

void Player::UnPause() {
  const int64_t now = static_cast<int64_t>(std::time(nullptr));
  const bool has_handler = url_handlers_ && url_handlers_->HandlerForUrl(current_song_.url());
  if (PlayerStreamExpire::NeedsRefresh(current_song_, has_handler, pause_started_sec_, now)) {
    pending_play_pause_ = false;
    pending_play_flags_ = GstEngine::Manual;
    pending_play_offset_ = static_cast<uint64_t>(std::max<int64_t>(0, engine_->position_nanosec()));
    if (UrlHandler *handler = url_handlers_->HandlerForUrl(current_song_.url())) {
      if (PlayerLoadResult::LoadingAsyncContains(loading_async_, current_song_.url())) {
        return;
      }
      const UrlHandler::LoadResult result = handler->Load(current_song_.url(), [this](const UrlHandler::LoadResult &async) {
        HandleLoadResult(async);
      });
      if (PlayerLoadResult::ShouldDeferEngineStart(result.type)) {
        PlayerLoadResult::LoadingAsyncInsert(&loading_async_, current_song_.url());
      } else {
        HandleLoadResult(result);
      }
      return;
    }
  }
  engine_->Unpause();
}

void Player::Previous() {
  if (!playlist_manager_) {
    return;
  }
  const int64_t now = static_cast<int64_t>(std::time(nullptr));
  if (PlayerPrevious::ShouldRestartTrack(menu_previous_mode_, last_previous_press_sec_, now)) {
    last_previous_press_sec_ = now;
    PlayAt(playlist_manager_->current_row());
    return;
  }
  last_previous_press_sec_ = now;
  if (PlayerPrevious::ShouldSeekToStart(menu_previous_mode_, engine_->position_nanosec())) {
    SeekTo(0);
    return;
  }
  MaybeEmitTrackSkipped();
  const Song previous = current_song_;
  const Song upcoming = playlist_manager_->active() ? playlist_manager_->active()->song(playlist_manager_->active()->PeekPreviousRow())
                                                    : Song();
  FinishCurrentPlayback();
  playlist_manager_->Previous();
  PlayCurrent(false, 0, SameAlbumFlags(GstEngine::Manual, previous, upcoming));
}

void Player::RestartOrPrevious() {
  if (engine_->position_nanosec() > 2 * 1000000000LL) {
    SeekTo(0);
  } else {
    Previous();
  }
}

void Player::SeekTo(int64_t seconds) { Seek(seconds * 1000000000LL); }

void Player::Seek(int64_t nanosec) {
  if (PlayerItemOptions::ShouldIgnoreSeek(current_song_)) {
    return;
  }
  const int64_t length = current_song_.length_nanosec() > 0 ? current_song_.length_nanosec() : engine_->length_nanosec();
  const int64_t clamped = PlayerSeekNotify::Clamp(nanosec, length);
  engine_->Seek(static_cast<uint64_t>(clamped));
  if (playlist_manager_ && playlist_manager_->active()) {
    playlist_manager_->active()->UpdateScrobblePoint(clamped);
  }
  PositionChanged.Emit(clamped, current_song_.length_nanosec());
  if (PlayerSeekNotify::ShouldRefreshNowPlaying(clamped, length)) {
    NowPlayingRefresh.Emit(current_song_);
  }
}

void Player::SeekForward() { SeekTo(engine_->position_nanosec() / 1000000000LL + seek_step_sec_); }

void Player::SeekBackward() {
  int64_t position = engine_->position_nanosec() / 1000000000LL - seek_step_sec_;
  SeekTo(position < 0 ? 0 : position);
}

void Player::SetVolume(unsigned volume) {
  volume_ = std::min(volume, 100u);
  engine_->SetVolumeSW(volume_);
  VolumeChanged.Emit(volume_);
}

void Player::VolumeUp() { SetVolume(volume_ + volume_increment_); }

void Player::VolumeDown() { SetVolume(volume_ > volume_increment_ ? volume_ - volume_increment_ : 0); }

void Player::Mute() {
  if (volume_ > 0) {
    volume_before_mute_ = volume_;
    SetVolume(0);
  } else {
    SetVolume(volume_before_mute_);
  }
}

void Player::PlayAt(int index, bool pause, uint64_t offset_nanosec) {
  int flags = GstEngine::Manual;
  if (playlist_manager_ && playlist_manager_->active()) {
    flags = SameAlbumFlags(flags, current_song_, playlist_manager_->active()->song(index));
  }
  MaybeEmitTrackSkipped();
  if (playlist_manager_) {
    playlist_manager_->SetCurrentRow(index);
  }
  PlayCurrent(pause, offset_nanosec, flags);
}

void Player::PlayPlaylist(const std::string &name) {
  if (playlist_manager_) {
    playlist_manager_->SetCurrentPlaylist(name);
  }
  Play();
}

void Player::ShowOSD() { ForceShowOSD.Emit(current_song_, false); }

void Player::TogglePrettyOSD() { ForceShowOSD.Emit(current_song_, true); }

void Player::PlayCurrent(bool pause, uint64_t offset_nanosec, int track_change_flags) {
  if (!playlist_manager_) {
    return;
  }
  FinishCurrentPlayback();
  current_song_ = playlist_manager_->current_song();
  PlayLoadedSong(pause, track_change_flags, offset_nanosec);
}

void Player::FinishCurrentPlayback() {
  if (finished_current_) {
    return;
  }
  finished_current_ = true;
  int64_t listened = engine_ ? engine_->position_nanosec() : 0;
  if (listened <= 0 && current_song_.length_nanosec() > 0) {
    listened = current_song_.length_nanosec();
  }
  PlaybackFinished.Emit(current_song_, listened);
}

void Player::StartEnginePlayback(bool pause, int track_change_flags, uint64_t offset_nanosec) {
  const bool intro = PlayerIntro::Active(playlist_manager_);
  if (intro) {
    track_change_flags |= GstEngine::Intro;
  }
  engine_->SetNextAlbum(current_song_.EffectiveAlbum());
  engine_->Load(current_song_.url(), current_song_.stream_url(), track_change_flags, PlayerIntro::HasForcedEnd(current_song_, intro),
                current_song_.beginning_nanosec(), PlayerIntro::EffectiveEndNanosec(current_song_, intro),
                current_song_.ebur128_integrated_loudness_lufs());
  engine_->SetCurrentAlbum(current_song_.EffectiveAlbum());
  engine_->Play(pause, offset_nanosec);
  error_count_ = 0;
  if (greyout_ && playlist_manager_) {
    playlist_manager_->SongChangeRequestProcessed(current_song_.url(), true);
  }
  SongChanged.Emit(current_song_);
  ArmIntroTimeout();
}

void Player::HandleLoadResult(const UrlHandler::LoadResult &result) {
  const std::string media = result.media_url.empty() ? current_song_.url() : result.media_url;
  PlayerLoadResult::LoadingAsyncErase(&loading_async_, media);
  Playlist *playlist = playlist_manager_ ? playlist_manager_->active() : nullptr;
  const Song next = playlist ? playlist->PeekNextSong() : Song();
  const auto target = PlayerLoadResult::MatchMediaUrl(media, current_song_.url(), next.url());
  if (PlayerLoadResult::ShouldTreatAsError(result.type)) {
    if (target == PlayerLoadResult::Target::Current) {
      HandleEngineError(result.error.empty() ? "URL handler error" : result.error);
    }
    return;
  }
  if (PlayerLoadResult::ShouldAdvanceOnNoMoreTracks(result.type)) {
    if (target == PlayerLoadResult::Target::Current) {
      Advance(GstEngine::Auto);
    }
    return;
  }
  if (PlayerLoadResult::ShouldDeferEngineStart(result.type)) {
    PlayerLoadResult::LoadingAsyncInsert(&loading_async_, media);
    return;
  }
  if (target == PlayerLoadResult::Target::Next && playlist) {
    Song patched = next;
    PlayerLoadResult::Apply(&patched, result);
    playlist->UpdateRowMetadata(playlist->PeekNextRow(), patched);
    if (PlayerLoadResult::ShouldPreloadResolved(target, current_song_.is_module_music())) {
      engine_->StartPreloading(next.url(), patched.stream_url(), SongSegment::HasForcedEnd(patched), patched.beginning_nanosec(),
                               SongSegment::EffectiveEndNanosec(patched));
    }
    return;
  }
  if (target == PlayerLoadResult::Target::None) {
    return;
  }
  PlayerLoadResult::Apply(&current_song_, result);
  if (playlist) {
    playlist->MergeFromEngine(current_song_);
  }
  StartEnginePlayback(pending_play_pause_, pending_play_flags_, pending_play_offset_);
}

void Player::PlayLoadedSong(bool pause, int track_change_flags, uint64_t offset_nanosec) {
  finished_current_ = false;
  const bool intro = PlayerIntro::Active(playlist_manager_);
  if (intro) {
    track_change_flags |= GstEngine::Intro;
  }
  if (!current_song_.is_valid() && current_song_.url().empty()) {
    Stop();
    return;
  }
  pending_play_pause_ = pause;
  pending_play_flags_ = track_change_flags;
  pending_play_offset_ = offset_nanosec;
  if (url_handlers_) {
    if (UrlHandler *handler = url_handlers_->HandlerForUrl(current_song_.url())) {
      if (PlayerLoadResult::LoadingAsyncContains(loading_async_, current_song_.url())) {
        return;
      }
      const UrlHandler::LoadResult result = handler->Load(current_song_.url(), [this](const UrlHandler::LoadResult &async) {
        HandleLoadResult(async);
      });
      if (PlayerLoadResult::ShouldDeferEngineStart(result.type)) {
        PlayerLoadResult::LoadingAsyncInsert(&loading_async_, current_song_.url());
        return;
      }
      HandleLoadResult(result);
      return;
    }
  }
  StartEnginePlayback(pause, track_change_flags, offset_nanosec);
}

void Player::CancelIntroTimeout() {
  if (intro_timeout_id_ != 0) {
    g_source_remove(intro_timeout_id_);
    intro_timeout_id_ = 0;
  }
  ++intro_generation_;
}

void Player::ArmIntroTimeout() {
  CancelIntroTimeout();
  const PlaylistSequence::RepeatMode repeat = playlist_manager_ && playlist_manager_->active()
                                                 ? playlist_manager_->active()->repeat_mode()
                                                 : PlaylistSequence::RepeatMode::Off;
  if (!PlayerRepeat::IsIntro(repeat)) {
    return;
  }
  struct IntroJob {
    Player *player = nullptr;
    int generation = 0;
  };
  auto *job = new IntroJob;
  job->player = this;
  job->generation = intro_generation_;
  intro_timeout_id_ = g_timeout_add_full(
      G_PRIORITY_DEFAULT, PlayerRepeat::IntroTimeoutMs(),
      +[](gpointer data) -> gboolean {
        auto *job = static_cast<IntroJob *>(data);
        Player *self = job->player;
        if (!self) {
          return G_SOURCE_REMOVE;
        }
        self->intro_timeout_id_ = 0;
        if (self->intro_generation_ != job->generation) {
          return G_SOURCE_REMOVE;
        }
        const PlaylistSequence::RepeatMode mode = self->playlist_manager_ && self->playlist_manager_->active()
                                                     ? self->playlist_manager_->active()->repeat_mode()
                                                     : PlaylistSequence::RepeatMode::Off;
        if (PlayerRepeat::IsIntro(mode)) {
          self->Advance(PlayerIntro::AdvanceFlags());
        }
        return G_SOURCE_REMOVE;
      },
      job, +[](gpointer data) { delete static_cast<IntroJob *>(data); });
}

void Player::HandleEngineMetadata(const Song &song) {
  Playlist *playlist = playlist_manager_ ? playlist_manager_->active() : nullptr;
  const Song next = playlist ? playlist->PeekNextSong() : Song();
  const auto target = PlayerNextMetadata::TargetForUrl(song.url(), current_song_.url(), current_song_.stream_url(), next.url(),
                                                       next.stream_url());
  if (PlayerNextMetadata::ShouldApplyToNext(target) && playlist) {
    playlist->UpdateRowMetadata(playlist->PeekNextRow(), song);
    return;
  }
  if (target == PlayerNextMetadata::Target::None) {
    return;
  }
  const Song before = current_song_;
  PlayerMetadataSync::Merge(&current_song_, song);
  if (playlist) {
    Song patch = current_song_;
    if (patch.url().empty()) {
      patch.set_url(before.url());
    }
    playlist->MergeFromEngine(patch);
  }
  if (PlayerMetadataSync::ShouldRefreshPlaylist(before, current_song_)) {
    SongChanged.Emit(current_song_);
  }
}

void Player::HandleEngineError(const std::string &error) {
  if (!error.empty()) {
    Error.Emit(error);
  }
  LogError("%s", error.c_str());
  if (greyout_ && playlist_manager_) {
    playlist_manager_->SongChangeRequestProcessed(current_song_.url(), false);
  }
  const PlaylistSequence::RepeatMode repeat = playlist_manager_ && playlist_manager_->active()
                                                 ? playlist_manager_->active()->repeat_mode()
                                                 : PlaylistSequence::RepeatMode::Off;
  const int rows = playlist_manager_ && playlist_manager_->active() ? playlist_manager_->active()->row_count() : 0;
  ++error_count_;
  if (PlayerErrorLoop::ShouldStopAutoAdvance(repeat, error_count_, rows) ||
      PlaylistBehaviour::ShouldStopAfterError(continue_on_error_, error_count_, rows)) {
    error_count_ = 0;
    Stop();
    return;
  }
  Next();
}

void Player::PreloadNext() {
  if (playlist_manager_) {
    playlist_manager_->RefillDynamic();
  }
  Song next_song;
  if (queue_ && !queue_->empty()) {
    next_song = queue_->songs().front();
  } else if (playlist_manager_) {
    next_song = playlist_manager_->PeekNextSong();
  }
  bool stop_after = stop_after_current_;
  if (playlist_manager_ && playlist_manager_->active()) {
    stop_after = PlayerRepeat::ShouldStopAfterTrack(playlist_manager_->active()->repeat_mode(), stop_after_current_,
                                                    playlist_manager_->active()->stop_after_row(),
                                                    playlist_manager_->active()->current_row());
  }
  if (!PlayerPreload::CanPreload(stop_after, next_song.is_valid() || !next_song.url().empty(),
                                 current_song_.is_module_music())) {
    return;
  }
  const bool same_album = current_song_.IsOnSameAlbum(next_song);
  if (PlayerPreload::ShouldAdvanceOnAboutToEnd(engine_->autocrossfade_enabled(), same_album, engine_->no_crossfade_same_album())) {
    Advance(GstEngine::Auto);
    return;
  }
  if (url_handlers_) {
    if (UrlHandler *handler = url_handlers_->HandlerForUrl(next_song.url())) {
      if (PlayerLoadResult::LoadingAsyncContains(loading_async_, next_song.url())) {
        return;
      }
      const UrlHandler::LoadResult result = handler->Load(next_song.url(), [this](const UrlHandler::LoadResult &async) {
        HandleLoadResult(async);
      });
      if (PlayerLoadResult::ShouldDeferEngineStart(result.type)) {
        PlayerLoadResult::LoadingAsyncInsert(&loading_async_, next_song.url());
        return;
      }
      HandleLoadResult(result);
      return;
    }
  }
  engine_->StartPreloading(next_song.url(), next_song.stream_url(), SongSegment::HasForcedEnd(next_song),
                           next_song.beginning_nanosec(), SongSegment::EffectiveEndNanosec(next_song));
}

void Player::HandleEngineState(EngineBase::State state) {
  StateChanged.Emit(state);
  switch (state) {
    case GstEngine::State::Playing:
      pause_started_sec_ = 0;
      Playing.Emit();
      break;
    case GstEngine::State::Paused:
      pause_started_sec_ = static_cast<int64_t>(std::time(nullptr));
      Paused.Emit();
      break;
    case GstEngine::State::Empty:
      Stopped.Emit();
      break;
    default:
      break;
  }
}

void Player::HandleTrackEnded() {
  CancelIntroTimeout();
  if (preloaded_) {
    preloaded_ = false;
    return;
  }
  if (PlayCountIncrement::ShouldIncrementOnTrackEnd(current_song_)) {
    if (playlist_manager_ && playlist_manager_->collection_backend()) {
      playlist_manager_->collection_backend()->IncrementPlayCount(current_song_.id());
    }
    TrackEndedPlaycount.Emit(current_song_);
  }
  FinishCurrentPlayback();
  const PlaylistSequence::RepeatMode repeat = playlist_manager_ && playlist_manager_->active()
                                                 ? playlist_manager_->active()->repeat_mode()
                                                 : PlaylistSequence::RepeatMode::Off;
  const int stop_after_row = playlist_manager_ && playlist_manager_->active() ? playlist_manager_->active()->stop_after_row() : -1;
  const int current_row = playlist_manager_ && playlist_manager_->active() ? playlist_manager_->active()->current_row() : -1;
  if (PlayerRepeat::ShouldStopAfterTrack(repeat, stop_after_current_, stop_after_row, current_row)) {
    if (PlayerStopAfter::ShouldPrepareResume(true) && playlist_manager_ && playlist_manager_->active()) {
      const int next = PlayerStopAfter::ResumeRow(playlist_manager_->active()->PeekNextRow());
      if (next >= 0) {
        playlist_manager_->active()->set_current_row(next);
      }
      playlist_manager_->active()->set_stop_after_row(-1);
    }
    stop_after_current_ = false;
    Stop();
    return;
  }
  if (playlist_manager_) {
    playlist_manager_->RefillDynamic();
  }
  Song next_song;
  if (queue_ && !queue_->empty()) {
    next_song = queue_->songs().front();
  } else if (playlist_manager_) {
    next_song = playlist_manager_->PeekNextSong();
  }
  if (!next_song.is_valid() && next_song.url().empty()) {
    PlaylistFinished.Emit();
    Stop();
    return;
  }
  Advance(GstEngine::Auto);
}
