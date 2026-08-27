#ifndef STRAWBERRY_GLOBALSHORTCUTS_H
#define STRAWBERRY_GLOBALSHORTCUTS_H

#include "config.h"
#include "core/signal.h"
#include "globalshortcuts/globalshortcut.h"
#include "globalshortcuts/globalshortcutsbackend.h"

#include <memory>
#include <string>
#include <vector>

class GlobalShortcutsManager {
 public:
  struct ShortcutDef {
    const char *id;
    const char *description;
    const char *default_key;
  };

  GlobalShortcutsManager();
  ~GlobalShortcutsManager();

  void Init();
  void ReloadSettings();
  void Raise();
  bool IsMacAccessibilityEnabled() const;
  void ShowMacAccessibilityDialog();
  const std::vector<std::unique_ptr<GlobalShortcut>> &shortcuts() const { return shortcuts_; }
  GlobalShortcut *ShortcutById(const std::string &id) const;
  void Emit(const std::string &id);
  bool HasActiveBackend(GlobalShortcutsBackend::Type type) const;

  static const std::vector<ShortcutDef> &Catalog();
  static std::vector<std::string> ShortcutIds();
  static std::string CanonicalId(const std::string &id);
  static std::string LegacySettingsKey(const std::string &id);
  static std::string FriendlyName(const std::string &id);
  static std::string DefaultKey(const std::string &id);

  Signal<> Play;
  Signal<> Pause;
  Signal<> PlayPause;
  Signal<> Stop;
  Signal<> StopAfter;
  Signal<> Next;
  Signal<> Previous;
  Signal<> RestartOrPrevious;
  Signal<> VolumeUp;
  Signal<> VolumeDown;
  Signal<> Mute;
  Signal<> SeekForward;
  Signal<> SeekBackward;
  Signal<> ShowHide;
  Signal<> ShowOSD;
  Signal<> TogglePrettyOSD;
  Signal<> CycleShuffle;
  Signal<> CycleRepeat;
  Signal<> ToggleScrobbling;
  Signal<> Love;

 private:
  void RegisterBackends();
  void UnregisterAll();
  void LoadShortcutKeys();

  std::vector<std::unique_ptr<GlobalShortcutsBackend>> backends_;
  std::vector<std::unique_ptr<GlobalShortcut>> shortcuts_;
  bool enabled_ = true;
};

#endif
