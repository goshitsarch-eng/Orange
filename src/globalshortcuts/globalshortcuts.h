#ifndef STRAWBERRY_GLOBALSHORTCUTS_H
#define STRAWBERRY_GLOBALSHORTCUTS_H

#include "core/signal.h"

#include <gio/gio.h>

#include <string>

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
  void GrabMediaKeys();
  static void OnMediaKey(GDBusProxy *proxy, const char *sender, const char *signal, GVariant *parameters, gpointer data);

  GDBusProxy *media_keys_ = nullptr;
  bool enabled_ = true;
};

#endif
