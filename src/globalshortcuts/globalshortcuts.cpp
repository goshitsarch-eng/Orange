#include "globalshortcuts/globalshortcuts.h"
#include "config.h"
#include "core/settings.h"
void GlobalShortcutsManager::Init() { ReloadSettings(); }
void GlobalShortcutsManager::ReloadSettings() {
  Settings s; s.BeginGroup("GlobalShortcuts");
  (void)s.BoolValue("enabled", true);
}
