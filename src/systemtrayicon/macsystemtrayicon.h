#ifndef STRAWBERRY_MACSYSTEMTRAYICON_H
#define STRAWBERRY_MACSYSTEMTRAYICON_H

#ifdef __APPLE__
#include "core/song.h"

class SystemTrayIcon;

class MacSystemTrayIcon {
 public:
  MacSystemTrayIcon();
  ~MacSystemTrayIcon();

  void Setup(SystemTrayIcon *tray);
  void SetNowPlaying(const Song &song);
  void ClearNowPlaying();

 private:
  void Rebuild();

  SystemTrayIcon *tray_ = nullptr;
  void *menu_ = nullptr;
  void *target_ = nullptr;
  void *now_playing_ = nullptr;
  void *artist_ = nullptr;
  void *title_ = nullptr;
};
#endif

#endif
