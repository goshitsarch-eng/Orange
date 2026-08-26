#include "settings/globalshortcutssettingspage.h"

#include "constants/globalshortcutssettings.h"
#include "settings/settingspage.h"
#include "ui/dialogs.h"

AdwPreferencesPage *GlobalShortcutsSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Shortcuts", "input-keyboard-symbolic");
  AdwPreferencesGroup *group = SettingsPage::AddGroup(page, "Global shortcuts");
  SettingsPage::AddToggle(group, settings, "enabled", "Enable global shortcuts", nullptr, true);
  SettingsPage::AddToggle(group, settings, GlobalShortcutsSettings::kUseKGlobalAccel, "Use KGlobalAccel", nullptr,
                          GlobalShortcutsSettings::kDefaultUseKGlobalAccel);
  SettingsPage::AddToggle(group, settings, GlobalShortcutsSettings::kUseX11, "Use X11 grab", nullptr, GlobalShortcutsSettings::kDefaultUseX11);
  SettingsPage::AddEntry(group, settings, "playpause", "Play / Pause", "MediaPlay");
  SettingsPage::AddEntry(group, settings, "next", "Next", "MediaNext");
  SettingsPage::AddEntry(group, settings, "previous", "Previous", "MediaPrevious");
  SettingsPage::AddButtonRow(group, "Grab play/pause shortcut", "Grab", [settings]() {
    Dialogs::GrabShortcut(nullptr, [settings](const std::string &accel) {
      settings->BeginGroup(GlobalShortcutsSettings::kSettingsGroup);
      settings->SetValue("playpause", accel);
      settings->Sync();
    });
  });
  return page;
}
