#include "settings/globalshortcutssettingspage.h"

#include "constants/globalshortcutssettings.h"
#include "globalshortcuts/globalshortcuts.h"
#include "settings/settingspage.h"
#include "ui/dialogs.h"

AdwPreferencesPage *GlobalShortcutsSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Shortcuts", "input-keyboard-symbolic");
  AdwPreferencesGroup *backends = SettingsPage::AddGroup(page, "Backends");
  SettingsPage::AddToggle(backends, settings, "enabled", "Enable global shortcuts", nullptr, true);
  SettingsPage::AddToggle(backends, settings, GlobalShortcutsSettings::kUseKGlobalAccel, "Use KGlobalAccel", nullptr,
                          GlobalShortcutsSettings::kDefaultUseKGlobalAccel);
  SettingsPage::AddToggle(backends, settings, GlobalShortcutsSettings::kUseX11, "Use X11 grab", nullptr,
                          GlobalShortcutsSettings::kDefaultUseX11);

  AdwPreferencesGroup *keys = SettingsPage::AddGroup(page, "Shortcuts");
  for (const auto &def : GlobalShortcutsManager::Catalog()) {
    SettingsPage::AddEntry(keys, settings, def.id, def.description, def.default_key);
  }
  SettingsPage::AddButtonRow(keys, "Grab play/pause shortcut", "Grab", [settings]() {
    Dialogs::GrabShortcut(nullptr, [settings](const std::string &accel) {
      settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
      settings->SetValue("play_pause", accel);
      settings->Sync();
    });
  });
  return page;
}
