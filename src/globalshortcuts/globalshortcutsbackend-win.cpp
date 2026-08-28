#include "globalshortcuts/globalshortcutsbackend-win.h"

#include "core/logging.h"
#include "globalshortcuts/globalshortcut.h"
#include "globalshortcuts/globalshortcuts.h"
#include "globalshortcuts/keymapper_win.h"

#ifdef _WIN32
#include <windows.h>
#endif

GlobalShortcutsBackendWin::GlobalShortcutsBackendWin(GlobalShortcutsManager *manager)
    : GlobalShortcutsBackend(manager, GlobalShortcutsBackend::Type::Win) {}

GlobalShortcutsBackendWin::~GlobalShortcutsBackendWin() { DoUnregister(); }

bool GlobalShortcutsBackendWin::IsAvailable() const {
#ifdef _WIN32
  return true;
#else
  return false;
#endif
}

bool GlobalShortcutsBackendWin::DoRegister() {
#ifdef _WIN32
  if (!manager_) {
    return false;
  }
  for (const GlobalShortcutsManager::ShortcutDef &def : GlobalShortcutsManager::Catalog()) {
    if (GlobalShortcut *shortcut = manager_->ShortcutById(def.id)) {
      AddShortcut(shortcut->id(), shortcut->key());
    }
  }
  return true;
#else
  return false;
#endif
}

void GlobalShortcutsBackendWin::DoUnregister() {
#ifdef _WIN32
  for (const auto &entry : ids_) {
    UnregisterHotKey(nullptr, entry.second);
  }
#endif
  ids_.clear();
}

bool GlobalShortcutsBackendWin::AddShortcut(const std::string &id, const std::string &key) {
#ifdef _WIN32
  UINT modifiers = 0;
  UINT vk = 0;
  if (!KeyMapperWin::Parse(key, &modifiers, &vk) || vk == 0) {
    return false;
  }
  const int hotkey_id = static_cast<int>(ids_.size()) + 1;
  if (!RegisterHotKey(nullptr, hotkey_id, modifiers, vk)) {
    LogWarning("RegisterHotKey failed for %s", id.c_str());
    return false;
  }
  ids_[id] = hotkey_id;
  return true;
#else
  (void)id;
  (void)key;
  return false;
#endif
}

void GlobalShortcutsBackendWin::RemoveShortcut(const std::string &id) {
#ifdef _WIN32
  auto it = ids_.find(id);
  if (it == ids_.end()) {
    return;
  }
  UnregisterHotKey(nullptr, it->second);
  ids_.erase(it);
#else
  (void)id;
#endif
}
