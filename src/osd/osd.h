#ifndef STRAWBERRY_OSD_H
#define STRAWBERRY_OSD_H

#include "core/song.h"

#include <gtk/gtk.h>

#include <string>
#include <vector>

class OSD {
 public:
  void ReloadSettings();
  void ShowMessage(const std::string &summary, const std::string &body, const std::string &icon = "audio-x-generic");
  void ShowPretty(const std::string &summary, const std::string &body, const std::vector<unsigned char> &art = {});
  void SongChanged(const Song &song, const std::vector<unsigned char> &art = {});
  bool enabled() const { return enabled_; }

 private:
  bool enabled_ = true;
  bool show_art_ = true;
  std::string type_ = "native";
  int timeout_ms_ = 4000;
  GtkWidget *pretty_window_ = nullptr;
  guint pretty_timeout_ = 0;
};

#endif
