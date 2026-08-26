#include "globalshortcuts/globalshortcuts.h"

#include "constants/globalshortcutssettings.h"
#include "core/settings.h"
#include "globalshortcuts/globalshortcutbinding.h"
#include "globalshortcuts/globalshortcutsbackend-kglobalaccel.h"
#include "globalshortcuts/globalshortcutsbackend-portal.h"
#include "globalshortcuts/globalshortcutsbackend-x11.h"

namespace {

const GlobalShortcutsManager::ShortcutDef kCatalog[] = {
    {"play", "Play", ""},
    {"pause", "Pause", ""},
    {"play_pause", "Play/Pause", "MediaPlay"},
    {"stop", "Stop", "MediaStop"},
    {"stop_after", "Stop playing after current track", ""},
    {"next_track", "Next track", "MediaNext"},
    {"prev_track", "Previous track", "MediaPrevious"},
    {"restart_or_prev_track", "Restart or previous track", ""},
    {"inc_volume", "Increase volume", ""},
    {"dec_volume", "Decrease volume", ""},
    {"mute", "Mute", ""},
    {"seek_forward", "Seek forward", ""},
    {"seek_backward", "Seek backward", ""},
    {"show_hide", "Show/Hide", ""},
    {"show_osd", "Show OSD", ""},
    {"toggle_pretty_osd", "Toggle Pretty OSD", ""},
    {"shuffle_mode", "Change shuffle mode", ""},
    {"repeat_mode", "Change repeat mode", ""},
    {"toggle_scrobbling", "Enable/disable scrobbling", ""},
    {"love", "Love", ""},
};

}  // namespace

GlobalShortcutsManager::GlobalShortcutsManager() {
  for (const ShortcutDef &def : kCatalog) {
    shortcuts_.push_back(std::make_unique<GlobalShortcut>(def.id, def.description, def.default_key));
  }
}

GlobalShortcutsManager::~GlobalShortcutsManager() { UnregisterAll(); }

const std::vector<GlobalShortcutsManager::ShortcutDef> &GlobalShortcutsManager::Catalog() {
  static const std::vector<ShortcutDef> catalog(std::begin(kCatalog), std::end(kCatalog));
  return catalog;
}

std::vector<std::string> GlobalShortcutsManager::ShortcutIds() {
  std::vector<std::string> ids;
  ids.reserve(Catalog().size());
  for (const ShortcutDef &def : Catalog()) {
    ids.emplace_back(def.id);
  }
  return ids;
}

std::string GlobalShortcutsManager::CanonicalId(const std::string &id) {
  if (id == "playpause") {
    return "play_pause";
  }
  if (id == "next") {
    return "next_track";
  }
  if (id == "previous") {
    return "prev_track";
  }
  if (id == "volume_up") {
    return "inc_volume";
  }
  if (id == "volume_down") {
    return "dec_volume";
  }
  return id;
}

std::string GlobalShortcutsManager::LegacySettingsKey(const std::string &id) {
  if (id == "play_pause") {
    return "playpause";
  }
  if (id == "next_track") {
    return "next";
  }
  if (id == "prev_track") {
    return "previous";
  }
  if (id == "inc_volume") {
    return "volume_up";
  }
  if (id == "dec_volume") {
    return "volume_down";
  }
  return {};
}

std::string GlobalShortcutsManager::FriendlyName(const std::string &id) {
  const std::string canonical = CanonicalId(id);
  for (const ShortcutDef &def : kCatalog) {
    if (canonical == def.id) {
      return def.description;
    }
  }
  return canonical;
}

std::string GlobalShortcutsManager::DefaultKey(const std::string &id) {
  const std::string canonical = CanonicalId(id);
  for (const ShortcutDef &def : kCatalog) {
    if (canonical == def.id) {
      return def.default_key;
    }
  }
  return {};
}

GlobalShortcut *GlobalShortcutsManager::ShortcutById(const std::string &id) const {
  const std::string canonical = CanonicalId(id);
  for (const auto &shortcut : shortcuts_) {
    if (shortcut->id() == canonical || shortcut->id() == id) {
      return shortcut.get();
    }
  }
  return nullptr;
}

void GlobalShortcutsManager::Emit(const std::string &id) {
  const std::string canonical = CanonicalId(id);
  if (GlobalShortcut *shortcut = ShortcutById(canonical)) {
    shortcut->Activated.Emit();
  }
  if (canonical == "play") {
    Play.Emit();
  } else if (canonical == "pause") {
    Pause.Emit();
  } else if (canonical == "play_pause") {
    PlayPause.Emit();
  } else if (canonical == "stop") {
    Stop.Emit();
  } else if (canonical == "stop_after") {
    StopAfter.Emit();
  } else if (canonical == "next_track") {
    Next.Emit();
  } else if (canonical == "prev_track") {
    Previous.Emit();
  } else if (canonical == "restart_or_prev_track") {
    RestartOrPrevious.Emit();
  } else if (canonical == "inc_volume") {
    VolumeUp.Emit();
  } else if (canonical == "dec_volume") {
    VolumeDown.Emit();
  } else if (canonical == "mute") {
    Mute.Emit();
  } else if (canonical == "seek_forward") {
    SeekForward.Emit();
  } else if (canonical == "seek_backward") {
    SeekBackward.Emit();
  } else if (canonical == "show_hide") {
    ShowHide.Emit();
  } else if (canonical == "show_osd") {
    ShowOSD.Emit();
  } else if (canonical == "toggle_pretty_osd") {
    TogglePrettyOSD.Emit();
  } else if (canonical == "shuffle_mode") {
    CycleShuffle.Emit();
  } else if (canonical == "repeat_mode") {
    CycleRepeat.Emit();
  } else if (canonical == "toggle_scrobbling") {
    ToggleScrobbling.Emit();
  } else if (canonical == "love") {
    Love.Emit();
  }
}

bool GlobalShortcutsManager::HasActiveBackend(GlobalShortcutsBackend::Type type) const {
  for (const auto &backend : backends_) {
    if (backend->type() == type && backend->is_active()) {
      return true;
    }
  }
  return false;
}

void GlobalShortcutsManager::Init() { ReloadSettings(); }

void GlobalShortcutsManager::Raise() {
  for (auto &backend : backends_) {
    if (backend->type() == GlobalShortcutsBackend::Type::Gnome && backend->is_active()) {
      backend->Unregister();
      backend->Register();
    }
  }
}

void GlobalShortcutsManager::UnregisterAll() {
  for (auto &backend : backends_) {
    backend->Unregister();
  }
  backends_.clear();
}

void GlobalShortcutsManager::RegisterBackends() {
  UnregisterAll();
  Settings s;
  s.BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  const bool use_kglobalaccel = s.BoolValue(GlobalShortcutsSettings::kUseKGlobalAccel, GlobalShortcutsSettings::kDefaultUseKGlobalAccel);
  const bool use_x11 = s.BoolValue(GlobalShortcutsSettings::kUseX11, GlobalShortcutsSettings::kDefaultUseX11);

  if (use_kglobalaccel) {
    auto kde = std::make_unique<GlobalShortcutsBackendKGlobalAccel>(this);
    if (kde->IsAvailable() && kde->Register()) {
      backends_.push_back(std::move(kde));
    }
  }

  auto gnome = std::make_unique<GlobalShortcutsBackendGnome>(this);
  if (gnome->IsAvailable() && gnome->Register()) {
    backends_.push_back(std::move(gnome));
  }

  auto portal = std::make_unique<GlobalShortcutsBackendPortal>(this);
  if (portal->IsAvailable() && portal->Register()) {
    backends_.push_back(std::move(portal));
  }

  if (use_x11 || backends_.empty()) {
    auto x11 = std::make_unique<GlobalShortcutsBackendX11>(this);
    if (x11->IsAvailable() && x11->Register()) {
      backends_.push_back(std::move(x11));
    }
  }
}

void GlobalShortcutsManager::LoadShortcutKeys() {
  Settings s;
  s.BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  for (auto &shortcut : shortcuts_) {
    const std::string alias = LegacySettingsKey(shortcut->id());
    const std::string key = GlobalShortcutBinding::ResolveStoredKey(s.Contains(shortcut->id()), s.Value(shortcut->id()),
                                                                    !alias.empty() && s.Contains(alias), alias.empty() ? std::string() : s.Value(alias),
                                                                    shortcut->default_key());
    shortcut->set_key(key);
  }
}

void GlobalShortcutsManager::ReloadSettings() {
  Settings s;
  s.BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  enabled_ = s.BoolValue("enabled", true);
  LoadShortcutKeys();
  if (!enabled_) {
    UnregisterAll();
    return;
  }
  RegisterBackends();
}
