#ifndef STRAWBERRY_SYSTEMTRAYICON_H
#define STRAWBERRY_SYSTEMTRAYICON_H
#include "core/song.h"
#include <gtk/gtk.h>
class SystemTrayIcon {
 public:
  SystemTrayIcon();
  ~SystemTrayIcon();
  void SetPlaying(bool playing);
  void SetProgress(int percentage);
  void SetNowPlaying(const Song &song);
  void SetupStatusNotifier();
  bool available() const { return available_; }
 private:
  GtkWidget *popover_ = nullptr;
  bool available_ = false;
  bool playing_ = false;
};
#endif
