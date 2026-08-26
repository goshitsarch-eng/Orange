#include "settings/notificationssettingspage.h"

#include "constants/notificationssettings.h"
#include "context/contextformattokens.h"
#include "core/application.h"
#include "core/song.h"
#include "osd/osdpretty.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "utilities/colorutils.h"

AdwPreferencesPage *NotificationsSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(OSDSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Notifications", "preferences-system-notifications-symbolic");
  AdwPreferencesGroup *osd = SettingsPage::AddGroup(page, Translations::CStr("On-screen display"));
  const std::string type_id = std::to_string(settings->IntValue(OSDSettings::kType, static_cast<int>(OSDSettings::kDefaultType)));
  SettingsPage::AddCombo(osd, settings, OSDSettings::kType, Translations::CStr("Notification type"),
                         {{"0", Translations::Tr("Disabled")},
                          {"1", Translations::Tr("Native")},
                          {"2", Translations::Tr("Tray popup")},
                          {"3", Translations::Tr("Pretty OSD")}},
                         type_id, [settings](const std::string &id) {
                           settings->BeginGroup(OSDSettings::kSettingsGroup);
                           settings->SetIntValue(OSDSettings::kType, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                           settings->Sync();
                         });
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
  SettingsPage::AddIntEntry(osd, settings, "posx", Translations::CStr("Pretty OSD X position"), 40);
  SettingsPage::AddIntEntry(osd, settings, "posy", Translations::CStr("Pretty OSD Y position"), 40);

  AdwPreferencesGroup *tokens1 = SettingsPage::AddGroup(page, Translations::CStr("Insert format token into line 1"));
  for (const auto &token : ContextFormatTokens::All()) {
    SettingsPage::AddButtonRow(tokens1, token.second.c_str(), token.first.c_str(), [settings, token]() {
      settings->BeginGroup(OSDSettings::kSettingsGroup);
      const std::string current = settings->Value(OSDSettings::kCustomText1);
      settings->SetValue(OSDSettings::kCustomText1, ContextFormatTokens::Insert(current, token.first));
      settings->Sync();
    });
  }
  AdwPreferencesGroup *tokens2 = SettingsPage::AddGroup(page, Translations::CStr("Insert format token into line 2"));
  for (const auto &token : ContextFormatTokens::All()) {
    SettingsPage::AddButtonRow(tokens2, token.second.c_str(), token.first.c_str(), [settings, token]() {
      settings->BeginGroup(OSDSettings::kSettingsGroup);
      const std::string current = settings->Value(OSDSettings::kCustomText2);
      settings->SetValue(OSDSettings::kCustomText2, ContextFormatTokens::Insert(current, token.first));
      settings->Sync();
    });
  }

  if (app) {
    SettingsPage::AddButtonRow(osd, Translations::CStr("Preview"), Translations::CStr("Show preview"), [settings, app]() {
      settings->BeginGroup(OSDSettings::kSettingsGroup);
      settings->Sync();
      app->osd()->ReloadSettings();
      Song song;
      song.set_valid(true);
      song.set_title("Roads");
      song.set_artist("Portishead");
      song.set_album("Dummy");
      const int type = settings->IntValue(OSDSettings::kType, static_cast<int>(OSDSettings::kDefaultType));
      const std::string line1 = settings->BoolValue(OSDSettings::kCustomTextEnabled, false)
                                    ? settings->Value(OSDSettings::kCustomText1, "%artist% - %title%")
                                    : "Portishead - Roads";
      const std::string line2 = settings->BoolValue(OSDSettings::kCustomTextEnabled, false) ? settings->Value(OSDSettings::kCustomText2, "%album%")
                                                                                           : "Dummy";
      app->osd()->ShowPreview(static_cast<OSDSettings::Type>(type), line1, line2, song);
    });
  }

  settings->BeginGroup(OSDPrettySettings::kSettingsGroup);
  AdwPreferencesGroup *pretty = SettingsPage::AddGroup(page, Translations::CStr("Pretty OSD"));
  const std::string blue = ColorUtils::HexToCss(OSDPrettySettings::kPresetBlue);
  const std::string red = ColorUtils::HexToCss(OSDPrettySettings::kPresetRed);
  SettingsPage::AddCombo(pretty, settings, nullptr, Translations::CStr("Color preset"),
                         {{"custom", Translations::Tr("Custom")},
                          {"blue", Translations::Tr("Last.fm blue")},
                          {"red", Translations::Tr("Red")}},
                         "custom", [settings, blue, red](const std::string &id) {
                           if (id == "custom") {
                             return;
                           }
                           settings->BeginGroup(OSDPrettySettings::kSettingsGroup);
                           settings->SetValue(OSDPrettySettings::kForegroundColor, "#ffffff");
                           settings->SetValue(OSDPrettySettings::kBackgroundColor, id == "red" ? red : blue);
                           settings->Sync();
                         });
  SettingsPage::AddColorButton(pretty, settings, OSDPrettySettings::kSettingsGroup, OSDPrettySettings::kForegroundColor,
                              Translations::CStr("Foreground color"), "#ffffff");
  SettingsPage::AddColorButton(pretty, settings, OSDPrettySettings::kSettingsGroup, OSDPrettySettings::kBackgroundColor,
                              Translations::CStr("Background color"), "#202020");
  SettingsPage::AddOpacityScale(pretty, settings, OSDPrettySettings::kSettingsGroup, OSDPrettySettings::kBackgroundOpacity,
                               Translations::CStr("Background opacity"), OSDPrettySettings::kDefaultBackgroundOpacity);
  SettingsPage::AddFontButton(pretty, settings, OSDPrettySettings::kSettingsGroup, OSDPrettySettings::kFont, Translations::CStr("Font"),
                             OSDPrettySettings::kDefaultFont);
  auto monitors = OSDPretty::MonitorChoices();
  if (monitors.size() == 1 && monitors.front().first.empty()) {
    monitors.front().second = Translations::Tr("Primary");
  }
  const std::string current_screen = settings->Value(OSDPrettySettings::kPopupScreen);
  SettingsPage::AddCombo(pretty, settings, nullptr, Translations::CStr("Popup screen"), monitors,
                         current_screen.empty() ? monitors.front().first : current_screen, [settings](const std::string &id) {
                           settings->BeginGroup(OSDPrettySettings::kSettingsGroup);
                           settings->SetValue(OSDPrettySettings::kPopupScreen, id);
                           settings->Sync();
                         });
  SettingsPage::AddToggle(pretty, settings, OSDPrettySettings::kDisableDuration, Translations::CStr("Disable timeout"), nullptr,
                          OSDPrettySettings::kDefaultDisableDuration);
  SettingsPage::AddToggle(pretty, settings, OSDPrettySettings::kFading, Translations::CStr("Fade the popup"), nullptr, true);
  SettingsPage::AddButtonRow(pretty, Translations::CStr("Position preview"), Translations::CStr("Show"), [settings, page]() {
    settings->BeginGroup(OSDPrettySettings::kSettingsGroup);
    settings->Sync();
    auto *preview = static_cast<OSDPretty *>(g_object_get_data(G_OBJECT(page), "pretty-preview"));
    if (!preview) {
      preview = new OSDPretty(OSDPretty::Mode::Draggable);
      g_object_set_data_full(G_OBJECT(page), "pretty-preview", preview, +[](gpointer data) { delete static_cast<OSDPretty *>(data); });
    }
    preview->ReloadSettings();
    preview->ShowMessage(Translations::Tr("OSD Preview"), Translations::Tr("Drag to reposition"));
  });

  settings->BeginGroup(DiscordRPCSettings::kSettingsGroup);
  AdwPreferencesGroup *discord = SettingsPage::AddGroup(page, Translations::CStr("Discord"));
  SettingsPage::AddToggle(discord, settings, DiscordRPCSettings::kEnabled, Translations::CStr("Enable Discord Rich Presence"), nullptr,
                          DiscordRPCSettings::kDefaultEnabled);
  const std::string discord_id =
      std::to_string(settings->IntValue(DiscordRPCSettings::kStatusDisplayType, static_cast<int>(DiscordRPCSettings::kDefaultStatusDisplayType)));
  SettingsPage::AddCombo(discord, settings, DiscordRPCSettings::kStatusDisplayType, Translations::CStr("Status display"),
                         {{"0", Translations::Tr("Application")},
                          {"1", Translations::Tr("Artist")},
                          {"2", Translations::Tr("Song")}},
                         discord_id, [settings](const std::string &id) {
                           settings->BeginGroup(DiscordRPCSettings::kSettingsGroup);
                           settings->SetIntValue(DiscordRPCSettings::kStatusDisplayType,
                                                 static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                           settings->Sync();
                         });
  return page;
}
