#include "settings/notificationssettingspage.h"

#include "constants/notificationssettings.h"
#include "context/contextformattokens.h"
#include "core/application.h"
#include "core/song.h"
#include "osd/osdpretty.h"
#include "settings/notificationscontrols.h"
#include "settings/notificationssettingslabels.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "utilities/colorutils.h"

AdwPreferencesPage *NotificationsSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(OSDSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Notifications", "preferences-system-notifications-symbolic");
  SettingsPage::AddDescription(SettingsPage::AddGroup(page), NotificationsSettingsLabels::Intro());
  AdwPreferencesGroup *osd = SettingsPage::AddGroup(page, NotificationsSettingsLabels::TypeGroup());
  const int type_value = settings->IntValue(OSDSettings::kType, static_cast<int>(OSDSettings::kDefaultType));
  const std::string type_id = std::to_string(type_value);
  GtkWidget *type = SettingsPage::AddCombo(osd, settings, OSDSettings::kType, NotificationsSettingsLabels::TypeGroup(),
                                           {{"0", NotificationsSettingsLabels::Disabled()},
                                            {"1", NotificationsSettingsLabels::Native()},
                                            {"2", NotificationsSettingsLabels::TrayPopup()},
                                            {"3", NotificationsSettingsLabels::Pretty()}},
                                           type_id, [settings](const std::string &id) {
                                             settings->BeginGroup(OSDSettings::kSettingsGroup);
                                             settings->SetIntValue(OSDSettings::kType, static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                                             settings->Sync();
                                           });
  AdwPreferencesGroup *general = SettingsPage::AddGroup(page, NotificationsSettingsLabels::General());
  GtkWidget *duration = SettingsPage::AddIntScale(general, settings, nullptr, nullptr, NotificationsSettingsLabels::PopupDuration(),
                                                  NotificationsControls::SecondsFromMs(settings->IntValue(OSDSettings::kTimeout, OSDSettings::kDefaultTimeout)),
                                                  NotificationsControls::MinSeconds(), NotificationsControls::MaxSeconds(), 1);
  g_object_set_data(G_OBJECT(duration), "settings", settings);
  g_signal_connect(duration, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer) {
                     auto *s = static_cast<Settings *>(g_object_get_data(G_OBJECT(range), "settings"));
                     if (!s) {
                       return;
                     }
                     s->BeginGroup(OSDSettings::kSettingsGroup);
                     s->SetIntValue(OSDSettings::kTimeout, NotificationsControls::MsFromSeconds(static_cast<int>(gtk_range_get_value(range))));
                     s->Sync();
                   }),
                   nullptr);
  settings->BeginGroup(OSDPrettySettings::kSettingsGroup);
  GtkWidget *disable_duration =
      SettingsPage::AddToggle(general, settings, OSDPrettySettings::kDisableDuration, NotificationsSettingsLabels::DisableDuration(), nullptr,
                              OSDPrettySettings::kDefaultDisableDuration, OSDPrettySettings::kSettingsGroup);
  settings->BeginGroup(OSDSettings::kSettingsGroup);
  gtk_widget_set_sensitive(duration, NotificationsControls::DurationSpinSensitive(type_value, adw_switch_row_get_active(ADW_SWITCH_ROW(disable_duration)))
                                         ? TRUE
                                         : FALSE);
  g_object_set_data(G_OBJECT(type), "duration-scale", duration);
  g_object_set_data(G_OBJECT(type), "disable-duration", disable_duration);
  g_object_set_data(G_OBJECT(disable_duration), "duration-scale", duration);
  g_object_set_data(G_OBJECT(disable_duration), "type-row", type);
  g_signal_connect(type, "notify::selected", G_CALLBACK(+[](AdwComboRow *combo, GParamSpec *, gpointer) {
                     auto *scale = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "duration-scale"));
                     auto *disable = GTK_WIDGET(g_object_get_data(G_OBJECT(combo), "disable-duration"));
                     if (scale && disable) {
                       gtk_widget_set_sensitive(scale, NotificationsControls::DurationSpinSensitive(static_cast<int>(adw_combo_row_get_selected(combo)),
                                                                                                    adw_switch_row_get_active(ADW_SWITCH_ROW(disable)))
                                                           ? TRUE
                                                           : FALSE);
                     }
                   }),
                   nullptr);
  g_signal_connect(disable_duration, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     auto *scale = GTK_WIDGET(g_object_get_data(G_OBJECT(row), "duration-scale"));
                     auto *combo = ADW_COMBO_ROW(g_object_get_data(G_OBJECT(row), "type-row"));
                     if (scale && combo) {
                       gtk_widget_set_sensitive(scale, NotificationsControls::DurationSpinSensitive(static_cast<int>(adw_combo_row_get_selected(combo)),
                                                                                                    adw_switch_row_get_active(row))
                                                           ? TRUE
                                                           : FALSE);
                     }
                   }),
                   nullptr);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowArt, NotificationsSettingsLabels::ShowArt(), nullptr, OSDSettings::kDefaultShowArt);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowOnVolumeChange, NotificationsSettingsLabels::ShowVolume(), nullptr,
                          OSDSettings::kDefaultShowOnVolumeChange);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowOnPlayModeChange, NotificationsSettingsLabels::ShowPlayMode(), nullptr,
                          OSDSettings::kDefaultShowOnPlayModeChange);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowOnPausePlayback, NotificationsSettingsLabels::ShowPause(), nullptr,
                          OSDSettings::kDefaultShowOnPausePlayback);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowOnResumePlayback, NotificationsSettingsLabels::ShowResume(), nullptr,
                          OSDSettings::kDefaultShowOnResumePlayback);
  AdwPreferencesGroup *custom = SettingsPage::AddGroup(page, NotificationsSettingsLabels::CustomGroup());
  SettingsPage::AddToggle(custom, settings, OSDSettings::kCustomTextEnabled, NotificationsSettingsLabels::CustomEnabled(), nullptr,
                          OSDSettings::kDefaultCustomTextEnabled);
  SettingsPage::AddEntry(custom, settings, OSDSettings::kCustomText1, NotificationsSettingsLabels::Summary());
  SettingsPage::AddEntry(custom, settings, OSDSettings::kCustomText2, NotificationsSettingsLabels::Body());
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
    SettingsPage::AddButtonRow(osd, NotificationsSettingsLabels::Preview(), NotificationsSettingsLabels::Preview(), [settings, app]() {
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
  SettingsPage::AddToggle(discord, settings, DiscordRPCSettings::kEnabled, NotificationsSettingsLabels::DiscordEnable(), nullptr,
                          DiscordRPCSettings::kDefaultEnabled);
  const std::string discord_id =
      std::to_string(settings->IntValue(DiscordRPCSettings::kStatusDisplayType, static_cast<int>(DiscordRPCSettings::kDefaultStatusDisplayType)));
  SettingsPage::AddCombo(discord, settings, DiscordRPCSettings::kStatusDisplayType, NotificationsSettingsLabels::DiscordListening(),
                         {{"0", NotificationsSettingsLabels::DiscordApp()},
                          {"1", NotificationsSettingsLabels::DiscordArtist()},
                          {"2", NotificationsSettingsLabels::DiscordSong()}},
                         discord_id, [settings](const std::string &id) {
                           settings->BeginGroup(DiscordRPCSettings::kSettingsGroup);
                           settings->SetIntValue(DiscordRPCSettings::kStatusDisplayType,
                                                 static_cast<int>(g_ascii_strtoll(id.c_str(), nullptr, 10)));
                           settings->Sync();
                         });
  return page;
}
