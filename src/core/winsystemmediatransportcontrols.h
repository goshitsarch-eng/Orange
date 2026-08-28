#ifndef STRAWBERRY_WINSYSTEMMEDIATRANSPORTCONTROLS_H
#define STRAWBERRY_WINSYSTEMMEDIATRANSPORTCONTROLS_H

#include "core/song.h"
#include "core/winsmtcstatus.h"
#include "engine/enginebase.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class Player;

class WinSystemMediaTransportControls {
 public:
  using Button = std::function<void(const std::string &)>;

  explicit WinSystemMediaTransportControls(Player *player);
  ~WinSystemMediaTransportControls();

  bool Initialize(void *hwnd);
  void EngineStateChanged(EngineBase::State state);
  void CurrentSongChanged(const Song &song);
  void AlbumCoverLoaded(const Song &song, const std::vector<unsigned char> &image);
  void HandleButtonPressed(int button);
  void set_button_callback(Button cb) { button_ = std::move(cb); }

 private:
  void UpdatePlaybackStatus(EngineBase::State state);
  void UpdateMetadata(const Song &song);
  void UpdateTimeline();
  void SetThumbnail(const std::vector<unsigned char> &image);
  void ClearThumbnail();
  void StartTimelineTimer();
  void StopTimelineTimer();

  Player *player_ = nullptr;
  bool initialized_ = false;
  bool ro_initialized_ = false;
  void *smtc_ = nullptr;
  void *smtc2_ = nullptr;
  void *updater_ = nullptr;
  void *button_handler_ = nullptr;
  int64_t button_pressed_token_ = 0;
  unsigned timeline_timer_ = 0;
  EngineBase::State state_ = EngineBase::State::Empty;
  std::string current_song_url_;
  int64_t current_duration_nanosec_ = 0;
  std::shared_ptr<bool> alive_ = std::make_shared<bool>(true);
  Button button_;
};

#endif
