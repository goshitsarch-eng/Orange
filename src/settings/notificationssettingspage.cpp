#include "settings/notificationssettingspage.h"

#include "constants/notificationssettings.h"
#include "settings/settingspage.h"

AdwPreferencesPage *NotificationsSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(OSDSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Notifications", "preferences-system-notifications-symbolic");
  AdwPreferencesGroup *osd = SettingsPage::AddGroup(page, "On-screen display");
  SettingsPage::AddIntEntry(osd, settings, OSDSettings::kType, "Type (0 disabled / 1 native / 2 tray / 3 pretty)",
                            static_cast<int>(OSDSettings::kDefaultType));
  SettingsPage::AddIntEntry(osd, settings, OSDSettings::kTimeout, "Timeout (ms)", OSDSettings::kDefaultTimeout);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowArt, "Show album art", nullptr, OSDSettings::kDefaultShowArt);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowOnVolumeChange, "Show on volume change", nullptr, OSDSettings::kDefaultShowOnVolumeChange);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowOnPlayModeChange, "Show on play mode change", nullptr,
                          OSDSettings::kDefaultShowOnPlayModeChange);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowOnPausePlayback, "Show on pause", nullptr, OSDSettings::kDefaultShowOnPausePlayback);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowOnResumePlayback, "Show on resume", nullptr, OSDSettings::kDefaultShowOnResumePlayback);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kCustomTextEnabled, "Custom notification text", nullptr, OSDSettings::kDefaultCustomTextEnabled);
  SettingsPage::AddEntry(osd, settings, OSDSettings::kCustomText1, "Custom text line 1");
  SettingsPage::AddEntry(osd, settings, OSDSettings::kCustomText2, "Custom text line 2");

  settings->BeginGroup(OSDPrettySettings::kSettingsGroup);
  AdwPreferencesGroup *pretty = SettingsPage::AddGroup(page, "Pretty OSD");
  SettingsPage::AddEntry(pretty, settings, OSDPrettySettings::kForegroundColor, "Foreground color");
  SettingsPage::AddEntry(pretty, settings, OSDPrettySettings::kBackgroundColor, "Background color");
  SettingsPage::AddEntry(pretty, settings, OSDPrettySettings::kBackgroundOpacity, "Background opacity", "0.85");
  SettingsPage::AddEntry(pretty, settings, OSDPrettySettings::kFont, "Font", OSDPrettySettings::kDefaultFont);
  SettingsPage::AddToggle(pretty, settings, OSDPrettySettings::kDisableDuration, "Disable timeout", nullptr, OSDPrettySettings::kDefaultDisableDuration);
  SettingsPage::AddToggle(pretty, settings, OSDPrettySettings::kFading, "Fade the popup", nullptr, true);

  settings->BeginGroup(DiscordRPCSettings::kSettingsGroup);
  AdwPreferencesGroup *discord = SettingsPage::AddGroup(page, "Discord");
  SettingsPage::AddToggle(discord, settings, DiscordRPCSettings::kEnabled, "Enable Discord Rich Presence", nullptr, DiscordRPCSettings::kDefaultEnabled);
  SettingsPage::AddIntEntry(discord, settings, DiscordRPCSettings::kStatusDisplayType, "Status display (0 app / 1 artist / 2 song)",
                            static_cast<int>(DiscordRPCSettings::kDefaultStatusDisplayType));
  return page;
}
