#ifndef STRAWBERRY_OSD_H
#define STRAWBERRY_OSD_H
#include "core/song.h"
#include <string>
class OSD {
 public:
  void ReloadSettings();
  void ShowMessage(const std::string &summary, const std::string &body, const std::string &icon = "audio-x-generic");
  void SongChanged(const Song &song);
  bool enabled() const { return enabled_; }
 private:
  bool enabled_ = true;
  bool show_art_ = true;
};
#endif
