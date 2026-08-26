#ifndef STRAWBERRY_GLOBALSHORTCUTS_H
#define STRAWBERRY_GLOBALSHORTCUTS_H

#include "config.h"
#include "core/signal.h"
#include "globalshortcuts/globalshortcut.h"

#include <memory>
#include <vector>

class GlobalShortcutsBackend;

class GlobalShortcutsManager {
 public:
  GlobalShortcutsManager();
  ~GlobalShortcutsManager();

  void Init();
  void ReloadSettings();
  const std::vector<std::unique_ptr<GlobalShortcut>> &shortcuts() const { return shortcuts_; }
  GlobalShortcut *ShortcutById(const std::string &id) const;

  Signal<> PlayPause;
  Signal<> Stop;
  Signal<> Next;
  Signal<> Previous;
  Signal<> VolumeUp;
  Signal<> VolumeDown;
  Signal<> ShowOSD;
  Signal<> Mute;

 private:
  void RegisterBackends();
  void UnregisterAll();

  std::vector<std::unique_ptr<GlobalShortcutsBackend>> backends_;
  std::vector<std::unique_ptr<GlobalShortcut>> shortcuts_;
  bool enabled_ = true;
};

#endif
