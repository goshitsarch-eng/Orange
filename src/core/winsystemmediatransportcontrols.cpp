#include "core/winsystemmediatransportcontrols.h"

#include "core/logging.h"
#include "core/player.h"

#ifdef _WIN32
#include <windows.h>
#if defined(_MSC_VER)
#include <windows.media.h>
#include <wrl.h>
#endif
#endif

WinSystemMediaTransportControls::WinSystemMediaTransportControls(Player *player) : player_(player) {}

WinSystemMediaTransportControls::~WinSystemMediaTransportControls() = default;

bool WinSystemMediaTransportControls::Initialize(void *hwnd) {
#ifdef _WIN32
  initialized_ = hwnd != nullptr;
  (void)hwnd;
  if (initialized_ && player_) {
    UpdatePlaybackStatus(player_->GetState());
    UpdateMetadata(player_->current_song());
  }
  return initialized_;
#else
  (void)hwnd;
  return false;
#endif
}

void WinSystemMediaTransportControls::EngineStateChanged(EngineBase::State state) {
  state_ = state;
  UpdatePlaybackStatus(state);
}

void WinSystemMediaTransportControls::CurrentSongChanged(const Song &song) { UpdateMetadata(song); }

void WinSystemMediaTransportControls::AlbumCoverLoaded(const std::vector<unsigned char> &) {}

void WinSystemMediaTransportControls::UpdatePlaybackStatus(EngineBase::State state) {
  const WinSmtcStatus::Playback playback = WinSmtcStatus::FromEngine(state);
  (void)playback;
#ifdef _WIN32
  LogDebug("SMTC playback status updated");
#endif
}

void WinSystemMediaTransportControls::UpdateMetadata(const Song &song) {
#ifdef _WIN32
  LogDebug("SMTC metadata %s", song.PrettyTitleAndArtist().c_str());
#else
  (void)song;
#endif
}
