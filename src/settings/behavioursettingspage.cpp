#include "settings/behavioursettingspage.h"

#include "constants/behavioursettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *BehaviourSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(BehaviourSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Behaviour", "preferences-system-symbolic");
  AdwPreferencesGroup *startup = SettingsPage::AddGroup(page, "Startup");
  SettingsPage::AddToggle(startup, settings, BehaviourSettings::kResumePlayback, "Resume playback on startup", nullptr,
                          BehaviourSettings::kDefaultResumePlayback);
  SettingsPage::AddCombo(startup, settings, BehaviourSettings::kStartupBehaviour, "Startup behaviour",
                         {{"1", "Remember"}, {"2", "Show"}, {"3", "Hide"}, {"4", "Show maximized"}, {"5", "Show minimized"}},
                         std::to_string(static_cast<int>(BehaviourSettings::kDefaultStartupBehaviour)));
  SettingsPage::AddEntry(startup, settings, BehaviourSettings::kLanguage, "Language");

  AdwPreferencesGroup *tray = SettingsPage::AddGroup(page, "System tray");
  SettingsPage::AddToggle(tray, settings, BehaviourSettings::kShowTrayIcon, "Show system tray icon", nullptr, BehaviourSettings::kDefaultShowTrayIcon);
  SettingsPage::AddToggle(tray, settings, BehaviourSettings::kKeepRunning, "Keep running in the tray when the window is closed", nullptr,
                          BehaviourSettings::kDefaultKeepRunning);
  SettingsPage::AddToggle(tray, settings, BehaviourSettings::kTrayIconProgress, "Show progress on the tray icon", nullptr,
                          BehaviourSettings::kDefaultTrayIconProgress);
  SettingsPage::AddToggle(tray, settings, BehaviourSettings::kTaskbarProgress, "Show progress on the taskbar", nullptr,
                          BehaviourSettings::kDefaultTaskbarProgress);
  SettingsPage::AddToggle(tray, settings, BehaviourSettings::kPlayingWidget, "Show playing widget", nullptr, BehaviourSettings::kDefaultPlayingWidget);

  AdwPreferencesGroup *playback = SettingsPage::AddGroup(page, "Playback");
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kMenuPlayMode, "Menu play mode",
                            {{"1", "Never"}, {"2", "If stopped"}, {"3", "Always"}},
                            static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode));
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kMenuPreviousMode, "Previous mode",
                            {{"1", "Don't restart"}, {"2", "Restart"}},
                            static_cast<int>(BehaviourSettings::kDefaultMenuPreviousMode));
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kDoubleClickAddMode, "Double-click add",
                            {{"1", "Append"}, {"2", "Enqueue"}, {"3", "Load"}, {"4", "Open in new playlist"}},
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickAddMode));
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kDoubleClickPlayMode, "Double-click play",
                            {{"1", "Never"}, {"2", "If stopped"}, {"3", "Always"}},
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickPlayMode));
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kDoubleClickPlaylistAddMode,
                            "Double-click playlist", {{"1", "Play"}, {"2", "Enqueue"}},
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickPlaylistAddMode));
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kSeekStepSec, "Seek step (seconds)", BehaviourSettings::kDefaultSeekStepSec);
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kVolumeIncrement, "Volume increment",
                            static_cast<int>(BehaviourSettings::kDefaultVolumeIncrement));
  return page;
}
