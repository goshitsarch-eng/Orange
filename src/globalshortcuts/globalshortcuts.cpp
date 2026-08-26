#include "globalshortcuts/globalshortcuts.h"

#include "core/settings.h"
#include "globalshortcuts/globalshortcutsbackend-kglobalaccel.h"
#include "globalshortcuts/globalshortcutsbackend-x11.h"

GlobalShortcutsManager::GlobalShortcutsManager() {
  shortcuts_.push_back(std::make_unique<GlobalShortcut>("playpause", "Play/Pause", "MediaPlay"));
  shortcuts_.push_back(std::make_unique<GlobalShortcut>("stop", "Stop", "MediaStop"));
  shortcuts_.push_back(std::make_unique<GlobalShortcut>("next", "Next", "MediaNext"));
  shortcuts_.push_back(std::make_unique<GlobalShortcut>("previous", "Previous", "MediaPrevious"));
  shortcuts_.push_back(std::make_unique<GlobalShortcut>("volume_up", "Volume up", "VolumeUp"));
  shortcuts_.push_back(std::make_unique<GlobalShortcut>("volume_down", "Volume down", "VolumeDown"));
  shortcuts_.push_back(std::make_unique<GlobalShortcut>("mute", "Mute", "Mute"));
  shortcuts_.push_back(std::make_unique<GlobalShortcut>("show_osd", "Show OSD", ""));
}

GlobalShortcutsManager::~GlobalShortcutsManager() { UnregisterAll(); }

GlobalShortcut *GlobalShortcutsManager::ShortcutById(const std::string &id) const {
  for (const auto &shortcut : shortcuts_) {
    if (shortcut->id() == id) {
      return shortcut.get();
    }
  }
  return nullptr;
}

void GlobalShortcutsManager::Init() { ReloadSettings(); }

void GlobalShortcutsManager::UnregisterAll() {
  for (auto &backend : backends_) {
    backend->Unregister();
  }
  backends_.clear();
}

void GlobalShortcutsManager::RegisterBackends() {
  UnregisterAll();
  auto gnome = std::make_unique<GlobalShortcutsBackendKGlobalAccel>(this);
  if (gnome->IsAvailable() && gnome->Register()) {
    backends_.push_back(std::move(gnome));
    return;
  }
  auto x11 = std::make_unique<GlobalShortcutsBackendX11>(this);
  if (x11->IsAvailable()) {
    x11->Register();
    backends_.push_back(std::move(x11));
  }
}

void GlobalShortcutsManager::ReloadSettings() {
  Settings s;
  s.BeginGroup("GlobalShortcuts");
  enabled_ = s.BoolValue("enabled", true);
  for (auto &shortcut : shortcuts_) {
    shortcut->set_key(s.Value(shortcut->id(), shortcut->default_key()));
  }
  if (!enabled_) {
    UnregisterAll();
    return;
  }
  RegisterBackends();
}
