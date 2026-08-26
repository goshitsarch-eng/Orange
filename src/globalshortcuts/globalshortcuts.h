#ifndef STRAWBERRY_GLOBALSHORTCUTS_H
#define STRAWBERRY_GLOBALSHORTCUTS_H

#include "config.h"
#include "core/signal.h"

#include <gio/gio.h>

#include <string>
#include <vector>

class GlobalShortcutsManager {
 public:
  GlobalShortcutsManager();
  ~GlobalShortcutsManager();

  void Init();
  void ReloadSettings();

  Signal<> PlayPause;
  Signal<> Stop;
  Signal<> Next;
  Signal<> Previous;
  Signal<> VolumeUp;
  Signal<> VolumeDown;
  Signal<> ShowOSD;
  Signal<> Mute;

 private:
  bool GrabMediaKeys();
  void UngrabAll();
  void GrabX11Keys(bool include_media);
  static void OnMediaKey(GDBusProxy *proxy, const char *sender, const char *signal, GVariant *parameters, gpointer data);
#ifdef HAVE_X11
  static gboolean OnX11Fd(gint fd, GIOCondition condition, gpointer data);
  void HandleX11Event();
  unsigned long KeysymFromName(const std::string &name) const;
#endif

  GDBusProxy *media_keys_ = nullptr;
  bool enabled_ = true;
#ifdef HAVE_X11
  void *x11_display_ = nullptr;
  guint x11_watch_id_ = 0;
  std::vector<unsigned int> x11_codes_;
#endif
};

#endif
