#include "settings/notificationssettingspage.h"

#include "constants/notificationssettings.h"
#include "settings/settingspage.h"
#include "translations/translations.h"

AdwPreferencesPage *NotificationsSettingsPage::Create(Settings *settings, Application *) {
  settings->BeginGroup(OSDSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage(Translations::CStr("Notifications"), "preferences-system-notifications-symbolic");
  AdwPreferencesGroup *osd = SettingsPage::AddGroup(page, Translations::CStr("On-screen display"));
  SettingsPage::AddIntEntry(osd, settings, OSDSettings::kType, Translations::CStr("Type (0 disabled / 1 native / 2 tray / 3 pretty)"),
                            static_cast<int>(OSDSettings::kDefaultType));
  SettingsPage::AddIntEntry(osd, settings, OSDSettings::kTimeout, Translations::CStr("Timeout (ms)"), OSDSettings::kDefaultTimeout);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowArt, Translations::CStr("Show album art"), nullptr, OSDSettings::kDefaultShowArt);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowOnVolumeChange, Translations::CStr("Show on volume change"), nullptr,
                          OSDSettings::kDefaultShowOnVolumeChange);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowOnPlayModeChange, Translations::CStr("Show on play mode change"), nullptr,
                          OSDSettings::kDefaultShowOnPlayModeChange);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowOnPausePlayback, Translations::CStr("Show on pause"), nullptr,
                          OSDSettings::kDefaultShowOnPausePlayback);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kShowOnResumePlayback, Translations::CStr("Show on resume"), nullptr,
                          OSDSettings::kDefaultShowOnResumePlayback);
  SettingsPage::AddToggle(osd, settings, OSDSettings::kCustomTextEnabled, Translations::CStr("Custom notification text"), nullptr,
                          OSDSettings::kDefaultCustomTextEnabled);
  SettingsPage::AddEntry(osd, settings, OSDSettings::kCustomText1, Translations::CStr("Custom text line 1"));
  SettingsPage::AddEntry(osd, settings, OSDSettings::kCustomText2, Translations::CStr("Custom text line 2"));

  settings->BeginGroup(OSDPrettySettings::kSettingsGroup);
  AdwPreferencesGroup *pretty = SettingsPage::AddGroup(page, Translations::CStr("Pretty OSD"));
  SettingsPage::AddEntry(pretty, settings, OSDPrettySettings::kForegroundColor, Translations::CStr("Foreground color"));
  SettingsPage::AddEntry(pretty, settings, OSDPrettySettings::kBackgroundColor, Translations::CStr("Background color"));
  SettingsPage::AddEntry(pretty, settings, OSDPrettySettings::kBackgroundOpacity, Translations::CStr("Background opacity"), "0.85");
  SettingsPage::AddEntry(pretty, settings, OSDPrettySettings::kFont, Translations::CStr("Font"), OSDPrettySettings::kDefaultFont);
  SettingsPage::AddToggle(pretty, settings, OSDPrettySettings::kDisableDuration, Translations::CStr("Disable timeout"), nullptr,
                          OSDPrettySettings::kDefaultDisableDuration);
  SettingsPage::AddToggle(pretty, settings, OSDPrettySettings::kFading, Translations::CStr("Fade the popup"), nullptr, true);

  settings->BeginGroup(DiscordRPCSettings::kSettingsGroup);
  AdwPreferencesGroup *discord = SettingsPage::AddGroup(page, Translations::CStr("Discord"));
  SettingsPage::AddToggle(discord, settings, DiscordRPCSettings::kEnabled, Translations::CStr("Enable Discord Rich Presence"), nullptr,
                          DiscordRPCSettings::kDefaultEnabled);
  SettingsPage::AddIntEntry(discord, settings, DiscordRPCSettings::kStatusDisplayType,
                            Translations::CStr("Status display (0 app / 1 artist / 2 song)"),
                            static_cast<int>(DiscordRPCSettings::kDefaultStatusDisplayType));
  return page;
}
