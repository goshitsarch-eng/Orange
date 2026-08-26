#include "settings/behavioursettingspage.h"

#include "constants/behavioursettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *BehaviourSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(BehaviourSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Behaviour", "preferences-system-symbolic");
  AdwPreferencesGroup *startup = SettingsPage::AddGroup(page, "Startup");
  SettingsPage::AddToggle(startup, settings, BehaviourSettings::kResumePlayback, "Resume playback on startup", nullptr,
                          BehaviourSettings::kDefaultResumePlayback);
  SettingsPage::AddIntEntry(startup, settings, BehaviourSettings::kStartupBehaviour, "Startup behaviour (1 remember / 2 show / 3 hide / 4 maximized / 5 minimized)",
                            static_cast<int>(BehaviourSettings::kDefaultStartupBehaviour));
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
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kMenuPlayMode, "Menu play mode (1 never / 2 if stopped / 3 always)",
                            static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode));
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kMenuPreviousMode, "Previous mode (1 don't restart / 2 restart)",
                            static_cast<int>(BehaviourSettings::kDefaultMenuPreviousMode));
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kDoubleClickAddMode, "Double-click add (1 append / 2 enqueue / 3 load / 4 new)",
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickAddMode));
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kDoubleClickPlayMode, "Double-click play (1 never / 2 if stopped / 3 always)",
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickPlayMode));
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kDoubleClickPlaylistAddMode, "Double-click playlist (1 play / 2 enqueue)",
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickPlaylistAddMode));
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kSeekStepSec, "Seek step (seconds)", BehaviourSettings::kDefaultSeekStepSec);
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kVolumeIncrement, "Volume increment",
                            static_cast<int>(BehaviourSettings::kDefaultVolumeIncrement));
  return page;
}
