#ifndef STRAWBERRY_WINSYSTEMMEDIATRANSPORTCONTROLS_H
#define STRAWBERRY_WINSYSTEMMEDIATRANSPORTCONTROLS_H

#include "core/song.h"
#include "core/winsmtcstatus.h"
#include "engine/enginebase.h"

#include <functional>
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
  void AlbumCoverLoaded(const std::vector<unsigned char> &image);
  void set_button_callback(Button cb) { button_ = std::move(cb); }

 private:
  void UpdatePlaybackStatus(EngineBase::State state);
  void UpdateMetadata(const Song &song);

  Player *player_ = nullptr;
  bool initialized_ = false;
  void *smtc_ = nullptr;
  EngineBase::State state_ = EngineBase::State::Empty;
  Button button_;
};

#endif
