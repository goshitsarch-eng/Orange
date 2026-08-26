#include "core/player.h"

#include <algorithm>

#include "constants/backendsettings.h"
#include "constants/behavioursettings.h"
#include "constants/playlistsettings.h"
#include "core/logging.h"
#include "core/playerresume.h"
#include "core/settings.h"
#include "core/urlhandlers.h"
#include "playlist/playlistbehaviour.h"
#include "playlist/playlistmanager.h"
#include "queue/queue.h"

Player::Player(TaskManager *task_manager, UrlHandlers *url_handlers, PlaylistManager *playlist_manager)
    : task_manager_(task_manager), url_handlers_(url_handlers), playlist_manager_(playlist_manager), engine_(std::make_unique<GstEngine>()) {}

void Player::Init() {
  engine_->Init();
  engine_->StateChanged.Connect([this](EngineBase::State state) { HandleEngineState(state); });
  engine_->TrackEnded.Connect([this]() { HandleTrackEnded(); });
  engine_->TrackAboutToEnd.Connect([this]() { PreloadNext(); });
  engine_->Error.Connect([this](const std::string &error) { HandleEngineError(error); });
  engine_->MetadataReceived.Connect([this](const Song &song) {
    if (current_song_.title().empty() && song.is_valid()) {
      if (!song.title().empty()) current_song_.set_title(song.title());
      if (!song.artist().empty()) current_song_.set_artist(song.artist());
      SongChanged.Emit(current_song_);
    }
  });
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
  if (playlist_manager_) {
    playlist_manager_->SetActiveToCurrent();
  }
  PlayAt(playlist_manager_ ? playlist_manager_->current_row() : 0, false);
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
      engine_->Unpause();
      break;
    default:
      Play();
      break;
  }
}

void Player::Pause() {
  engine_->Pause();
  if (playlist_manager_) {
    playlist_manager_->SetActivePaused();
  }
}

void Player::Stop(bool stop_after) {
  stop_after_current_ = stop_after;
  if (!stop_after) {
    FinishCurrentPlayback();
    engine_->Stop();
    if (playlist_manager_) {
      playlist_manager_->SetActiveStopped();
    }
    Stopped.Emit();
  }
}

void Player::StopAfterCurrent() { stop_after_current_ = !stop_after_current_; }

void Player::Next() {
  FinishCurrentPlayback();
  if (queue_ && !queue_->empty()) {
    current_song_ = queue_->TakeNext();
    PlayLoadedSong(false);
    return;
  }
  if (!playlist_manager_) {
    return;
  }
  playlist_manager_->Next();
  PlayCurrent(false);
}

void Player::Previous() {
  if (!playlist_manager_) {
    return;
  }
  if (engine_->position_nanosec() > 3 * 1000000000LL) {
    SeekTo(0);
    return;
  }
  FinishCurrentPlayback();
  playlist_manager_->Previous();
  PlayCurrent(false);
}

void Player::RestartOrPrevious() {
  if (engine_->position_nanosec() > 2 * 1000000000LL) {
    SeekTo(0);
  } else {
    Previous();
  }
}

void Player::SeekTo(int64_t seconds) { Seek(seconds * 1000000000LL); }

void Player::Seek(int64_t nanosec) { engine_->Seek(static_cast<uint64_t>(std::max<int64_t>(0, nanosec))); }

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
  if (playlist_manager_) {
    playlist_manager_->SetCurrentRow(index);
  }
  PlayCurrent(pause, offset_nanosec);
}

void Player::PlayPlaylist(const std::string &name) {
  if (playlist_manager_) {
    playlist_manager_->SetCurrentPlaylist(name);
  }
  Play();
}

void Player::ShowOSD() { ForceShowOSD.Emit(current_song_); }

void Player::PlayCurrent(bool pause, uint64_t offset_nanosec) {
  if (!playlist_manager_) {
    return;
  }
  FinishCurrentPlayback();
  current_song_ = playlist_manager_->current_song();
  PlayLoadedSong(pause, GstEngine::Manual, offset_nanosec);
}

namespace {

void ApplyLoadResult(Song *song, const UrlHandler::LoadResult &result) {
  if (!result.stream_url.empty()) {
    song->set_stream_url(result.stream_url);
  }
  if (result.filetype != Song::FileType::Unknown) {
    song->set_filetype(result.filetype);
  }
  if (result.samplerate > 0) {
    song->set_samplerate(result.samplerate);
  }
  if (result.bit_depth > 0) {
    song->set_bitdepth(result.bit_depth);
  }
  if (result.duration > 0) {
    song->set_length_nanosec(result.duration);
  }
  if (result.song.is_valid()) {
    if (!result.song.title().empty()) {
      song->set_title(result.song.title());
    }
    if (!result.song.artist().empty()) {
      song->set_artist(result.song.artist());
    }
    if (!result.song.album().empty()) {
      song->set_album(result.song.album());
    }
    if (!result.song.genre().empty()) {
      song->set_genre(result.song.genre());
    }
  }
}

}  // namespace

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

void Player::PlayLoadedSong(bool pause, int track_change_flags, uint64_t offset_nanosec) {
  finished_current_ = false;
  if (!current_song_.is_valid() && current_song_.url().empty()) {
    Stop();
    return;
  }
  if (url_handlers_) {
    if (UrlHandler *handler = url_handlers_->HandlerForUrl(current_song_.url())) {
      const UrlHandler::LoadResult result = handler->Load(current_song_.url(), [this, pause, track_change_flags, offset_nanosec](const UrlHandler::LoadResult &async) {
        ApplyLoadResult(&current_song_, async);
        if (!async.stream_url.empty()) {
          engine_->SetNextAlbum(current_song_.album());
          engine_->Load(current_song_.url(), async.stream_url, track_change_flags, false, current_song_.beginning_nanosec(),
                        current_song_.length_nanosec() > 0 ? current_song_.beginning_nanosec() + current_song_.length_nanosec() : -1,
                        current_song_.ebur128_integrated_loudness_lufs());
          engine_->SetCurrentAlbum(current_song_.album());
          engine_->Play(pause, offset_nanosec);
        }
      });
      if (result.type == UrlHandler::LoadResult::Type::TrackAvailable) {
        ApplyLoadResult(&current_song_, result);
      }
    }
  }
  engine_->SetNextAlbum(current_song_.album());
  engine_->Load(current_song_.url(), current_song_.stream_url(), track_change_flags, false, current_song_.beginning_nanosec(),
                current_song_.length_nanosec() > 0 ? current_song_.beginning_nanosec() + current_song_.length_nanosec() : -1,
                current_song_.ebur128_integrated_loudness_lufs());
  engine_->SetCurrentAlbum(current_song_.album());
  engine_->Play(pause, offset_nanosec);
  error_count_ = 0;
  if (greyout_ && playlist_manager_) {
    playlist_manager_->SongChangeRequestProcessed(current_song_.url(), true);
  }
  SongChanged.Emit(current_song_);
}

void Player::HandleEngineError(const std::string &error) {
  LogError("%s", error.c_str());
  if (greyout_ && playlist_manager_) {
    playlist_manager_->SongChangeRequestProcessed(current_song_.url(), false);
  }
  const int rows = playlist_manager_ && playlist_manager_->active() ? playlist_manager_->active()->row_count() : 0;
  ++error_count_;
  if (PlaylistBehaviour::ShouldStopAfterError(continue_on_error_, error_count_, rows)) {
    error_count_ = 0;
    Stop();
    return;
  }
  Next();
}

void Player::PreloadNext() {
  if (stop_after_current_) {
    return;
  }
  FinishCurrentPlayback();
  Song next_song;
  if (queue_ && !queue_->empty()) {
    next_song = queue_->songs().front();
  } else if (playlist_manager_) {
    playlist_manager_->RefillDynamic();
    next_song = playlist_manager_->PeekNextSong();
  }
  if (!next_song.is_valid() && next_song.url().empty()) {
    return;
  }
  if (queue_ && !queue_->empty()) {
    current_song_ = queue_->TakeNext();
  } else if (playlist_manager_) {
    playlist_manager_->Next();
    current_song_ = playlist_manager_->current_song();
  }
  preloaded_ = true;
  PlayLoadedSong(false, GstEngine::Auto);
}

void Player::HandleEngineState(EngineBase::State state) {
  StateChanged.Emit(state);
  switch (state) {
    case GstEngine::State::Playing:
      Playing.Emit();
      break;
    case GstEngine::State::Paused:
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
  if (preloaded_) {
    preloaded_ = false;
    return;
  }
  FinishCurrentPlayback();
  if (stop_after_current_) {
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
  Next();
}
