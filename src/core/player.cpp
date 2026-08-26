#include "core/player.h"

#include <algorithm>

#include "core/logging.h"
#include "core/settings.h"
#include "core/urlhandlers.h"
#include "playlist/playlistmanager.h"
#include "queue/queue.h"

Player::Player(TaskManager *task_manager, UrlHandlers *url_handlers, PlaylistManager *playlist_manager)
    : task_manager_(task_manager), url_handlers_(url_handlers), playlist_manager_(playlist_manager), engine_(std::make_unique<GstEngine>()) {}

void Player::Init() {
  engine_->Init();
  engine_->StateChanged.Connect([this](GstEngine::State state) { HandleEngineState(state); });
  engine_->TrackEnded.Connect([this]() { HandleTrackEnded(); });
  engine_->Error.Connect([](const std::string &error) { LogError("%s", error.c_str()); });
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

GstEngine::State Player::GetState() const { return engine_->state(); }

void Player::ReloadSettings() {
  Settings settings;
  settings.BeginGroup("Behaviour");
  seek_step_sec_ = settings.IntValue("seekstep", 10);
  volume_increment_ = static_cast<unsigned>(settings.IntValue("volumeincrement", 5));
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

void Player::Play() { PlayAt(playlist_manager_ ? playlist_manager_->current_row() : 0, false); }

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

void Player::Pause() { engine_->Pause(); }

void Player::Stop(bool stop_after) {
  stop_after_current_ = stop_after;
  if (!stop_after) {
    engine_->Stop();
    Stopped.Emit();
  }
}

void Player::StopAfterCurrent() { stop_after_current_ = !stop_after_current_; }

void Player::Next() {
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

void Player::SeekTo(int64_t seconds) { engine_->Seek(static_cast<uint64_t>(seconds) * 1000000000ULL); }

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

void Player::PlayAt(int index, bool pause) {
  if (playlist_manager_) {
    playlist_manager_->SetCurrentRow(index);
  }
  PlayCurrent(pause);
}

void Player::PlayPlaylist(const std::string &name) {
  if (playlist_manager_) {
    playlist_manager_->SetCurrentPlaylist(name);
  }
  Play();
}

void Player::ShowOSD() { ForceShowOSD.Emit(current_song_); }

void Player::PlayCurrent(bool pause) {
  if (!playlist_manager_) {
    return;
  }
  current_song_ = playlist_manager_->current_song();
  PlayLoadedSong(pause);
}

void Player::PlayLoadedSong(bool pause) {
  if (!current_song_.is_valid() && current_song_.url().empty()) {
    Stop();
    return;
  }
  if (url_handlers_) {
    if (UrlHandler *handler = url_handlers_->HandlerForUrl(current_song_.url())) {
      const UrlHandler::LoadResult result = handler->Load(current_song_.url(), [this, pause](const UrlHandler::LoadResult &async) {
        if (!async.stream_url.empty()) {
          current_song_.set_stream_url(async.stream_url);
          engine_->Load(current_song_.url(), async.stream_url, GstEngine::Manual, false, current_song_.beginning_nanosec(),
                        current_song_.length_nanosec() > 0 ? current_song_.beginning_nanosec() + current_song_.length_nanosec() : -1,
                        current_song_.ebur128_integrated_loudness_lufs());
          engine_->Play(pause, 0);
        }
      });
      if (result.type == UrlHandler::LoadResult::Type::TrackAvailable) {
        current_song_.set_stream_url(result.stream_url);
      }
    }
  }
  engine_->Load(current_song_.url(), current_song_.stream_url(), GstEngine::Manual, false, current_song_.beginning_nanosec(),
                current_song_.length_nanosec() > 0 ? current_song_.beginning_nanosec() + current_song_.length_nanosec() : -1,
                current_song_.ebur128_integrated_loudness_lufs());
  engine_->Play(pause, 0);
  SongChanged.Emit(current_song_);
}

void Player::HandleEngineState(GstEngine::State state) {
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
  if (stop_after_current_) {
    stop_after_current_ = false;
    Stop();
    return;
  }
  if (playlist_manager_) {
    playlist_manager_->RefillDynamic();
  }
  Next();
}
