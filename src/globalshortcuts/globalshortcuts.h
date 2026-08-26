#ifndef STRAWBERRY_GLOBALSHORTCUTS_H
#define STRAWBERRY_GLOBALSHORTCUTS_H
#include "core/signal.h"
#include <string>
class GlobalShortcutsManager {
 public:
  void Init();
  void ReloadSettings();
  Signal<> PlayPause;
  Signal<> Stop;
  Signal<> Next;
  Signal<> Previous;
  Signal<> VolumeUp;
  Signal<> VolumeDown;
};
#endif
