#include "settings/notificationssettingspage.h"

#include "constants/notificationssettings.h"
#include "settings/notificationpreviewsong.h"
#include "context/contextformattokens.h"
#include "core/application.h"
#include "core/song.h"
#include "osd/osdbase.h"
#include "osd/osdpretty.h"
#include "settings/notificationscontrols.h"
#include "settings/notificationssettingslabels.h"
#include "settings/settingspage.h"
#include "translations/translations.h"
#include "utilities/colorutils.h"

#include <string>
#include <utility>
#include <vector>

namespace {

struct NotificationSensitivity {
  GtkWidget *duration = nullptr;
  GtkWidget *disable_duration = nullptr;
  GtkWidget *art = nullptr;
  GtkWidget *general = nullptr;
  GtkWidget *pretty = nullptr;
  GtkWidget *custom = nullptr;
  GtkWidget *custom_toggle = nullptr;
  GtkWidget *text1 = nullptr;
  GtkWidget *text2 = nullptr;
  GtkWidget *preview = nullptr;
  GtkWidget *tokens1 = nullptr;
  GtkWidget *tokens2 = nullptr;
};

OSDSettings::Type ComboOsdType(AdwComboRow *combo) {
  return NotificationsControls::TypeFromSelected(
      static_cast<std::vector<std::string> *>(g_object_get_data(G_OBJECT(combo), "choice-ids")), adw_combo_row_get_selected(combo));
}

void ApplyNotificationSensitivity(AdwComboRow *combo) {
  auto *state = static_cast<NotificationSensitivity *>(g_object_get_data(G_OBJECT(combo), "sensitivity"));
  if (!state) {
    return;
  }
  const OSDSettings::Type type = ComboOsdType(combo);
  const bool disable_duration = state->disable_duration && adw_switch_row_get_active(ADW_SWITCH_ROW(state->disable_duration));
  if (state->general) {
    gtk_widget_set_sensitive(state->general, NotificationsControls::GeneralSensitive(type) ? TRUE : FALSE);
  }
  if (state->pretty) {
    gtk_widget_set_sensitive(state->pretty, NotificationsControls::PrettyGroupSensitive(type) ? TRUE : FALSE);
  }
  const bool custom_on = state->custom_toggle && adw_switch_row_get_active(ADW_SWITCH_ROW(state->custom_toggle));
  if (state->custom) {
    gtk_widget_set_sensitive(state->custom, NotificationsControls::CustomTextSensitive(type) ? TRUE : FALSE);
  }
  if (state->text1) {
    gtk_widget_set_sensitive(state->text1, NotificationsControls::CustomFieldsEnabled(type, custom_on) ? TRUE : FALSE);
  }
  if (state->text2) {
    gtk_widget_set_sensitive(state->text2, NotificationsControls::CustomFieldsEnabled(type, custom_on) ? TRUE : FALSE);
  }
  if (state->preview) {
    gtk_widget_set_sensitive(state->preview, NotificationsControls::PreviewEnabled(type, custom_on) ? TRUE : FALSE);
  }
  if (state->tokens1) {
    gtk_widget_set_sensitive(state->tokens1, NotificationsControls::TokenGroupsEnabled(type, custom_on) ? TRUE : FALSE);
  }
  if (state->tokens2) {
    gtk_widget_set_sensitive(state->tokens2, NotificationsControls::TokenGroupsEnabled(type, custom_on) ? TRUE : FALSE);
  }
  if (state->art) {
    gtk_widget_set_sensitive(state->art, NotificationsControls::ArtSensitive(type) ? TRUE : FALSE);
  }
  if (state->disable_duration) {
    gtk_widget_set_sensitive(state->disable_duration, NotificationsControls::DisableDurationSensitive(type) ? TRUE : FALSE);
  }
  if (state->duration) {
    gtk_widget_set_sensitive(state->duration, NotificationsControls::DurationSpinSensitive(type, disable_duration) ? TRUE : FALSE);
  }
}

void UpdatePrettyPreview(GtkWidget *page, AdwComboRow *type) {
  if (!page || !type) {
    return;
  }
  const bool show = NotificationsControls::ShouldShowPrettyPreview(gtk_widget_get_mapped(page), ComboOsdType(type));
  auto *preview = static_cast<OSDPretty *>(g_object_get_data(G_OBJECT(page), "pretty-preview"));
  if (!show) {
    if (preview) {
      preview->Hide();
    }
    return;
  }
  if (!preview) {
    preview = new OSDPretty(OSDPretty::Mode::Draggable);
    g_object_set_data_full(G_OBJECT(page), "pretty-preview", preview, +[](gpointer data) { delete static_cast<OSDPretty *>(data); });
  }
  preview->ReloadSettings();
  preview->ShowMessage(Translations::Tr("OSD Preview"), Translations::Tr("Drag to reposition"));
}

}  // namespace

AdwPreferencesPage *NotificationsSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(OSDSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Notifications", "preferences-system-notifications-symbolic");
  SettingsPage::AddDescription(SettingsPage::AddGroup(page), NotificationsSettingsLabels::Intro());
  AdwPreferencesGroup *osd = SettingsPage::AddGroup(page, NotificationsSettingsLabels::TypeGroup());
  const bool native = app && app->osd() ? app->osd()->SupportsNativeNotifications() : true;
  const bool tray = app && app->osd() ? app->osd()->SupportsTrayPopups() : false;
  const bool pretty_ok = OSDBase::SupportsOSDPretty();
  const int type_value = settings->IntValue(OSDSettings::kType, static_cast<int>(OSDSettings::kDefaultType));
  const OSDSettings::Type effective = NotificationsControls::EffectiveType(type_value, native, tray, pretty_ok);
  if (effective != static_cast<OSDSettings::Type>(type_value)) {
    settings->SetIntValue(OSDSettings::kType, static_cast<int>(effective));
  }
  std::vector<std::pair<std::string, std::string>> type_choices;
  for (const auto &choice : NotificationsControls::AvailableTypes(native, tray, pretty_ok)) {
    type_choices.emplace_back(choice.first, choice.second);
  }
  GtkWidget *type = SettingsPage::AddCombo(osd, settings, OSDSettings::kType, NotificationsSettingsLabels::TypeGroup(), type_choices,
                                           std::to_string(static_cast<int>(effective)), [settings](const std::string &id) {
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
  GtkWidget *art = SettingsPage::AddToggle(general, settings, OSDSettings::kShowArt, NotificationsSettingsLabels::ShowArt(), nullptr,
                                           OSDSettings::kDefaultShowArt);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowOnVolumeChange, NotificationsSettingsLabels::ShowVolume(), nullptr,
                          OSDSettings::kDefaultShowOnVolumeChange);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowOnPlayModeChange, NotificationsSettingsLabels::ShowPlayMode(), nullptr,
                          OSDSettings::kDefaultShowOnPlayModeChange);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowOnPausePlayback, NotificationsSettingsLabels::ShowPause(), nullptr,
                          OSDSettings::kDefaultShowOnPausePlayback);
  SettingsPage::AddToggle(general, settings, OSDSettings::kShowOnResumePlayback, NotificationsSettingsLabels::ShowResume(), nullptr,
                          OSDSettings::kDefaultShowOnResumePlayback);
  AdwPreferencesGroup *custom = SettingsPage::AddGroup(page, NotificationsSettingsLabels::CustomGroup());
  GtkWidget *custom_toggle =
      SettingsPage::AddToggle(custom, settings, OSDSettings::kCustomTextEnabled, NotificationsSettingsLabels::CustomEnabled(), nullptr,
                              OSDSettings::kDefaultCustomTextEnabled);
  GtkWidget *text1 = SettingsPage::AddEntry(custom, settings, OSDSettings::kCustomText1, NotificationsSettingsLabels::Summary());
  GtkWidget *text2 = SettingsPage::AddEntry(custom, settings, OSDSettings::kCustomText2, NotificationsSettingsLabels::Body());
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

  GtkWidget *preview = nullptr;
  if (app) {
    preview = SettingsPage::AddButtonRow(osd, NotificationsSettingsLabels::Preview(), NotificationsSettingsLabels::Preview(), [settings, app]() {
      settings->BeginGroup(OSDSettings::kSettingsGroup);
      settings->Sync();
      app->osd()->ReloadSettings();
      const SongList playlist_songs = app->playlist_manager() && app->playlist_manager()->current()
                                          ? app->playlist_manager()->current()->songs()
                                          : SongList{};
      const Song song = NotificationPreviewSong::FromPlaylist(playlist_songs);
      const int type = settings->IntValue(OSDSettings::kType, static_cast<int>(OSDSettings::kDefaultType));
      const std::string stored1 = settings->Value(OSDSettings::kCustomText1);
      const std::string stored2 = settings->Value(OSDSettings::kCustomText2);
      app->osd()->ShowPreview(static_cast<OSDSettings::Type>(type), stored1.empty() ? "%artist% - %title%" : stored1,
                              stored2.empty() ? "%album%" : stored2, song);
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
  auto *sensitivity = new NotificationSensitivity();
  sensitivity->duration = duration;
  sensitivity->disable_duration = disable_duration;
  sensitivity->art = art;
  sensitivity->general = GTK_WIDGET(general);
  sensitivity->pretty = GTK_WIDGET(pretty);
  sensitivity->custom = GTK_WIDGET(custom);
  sensitivity->custom_toggle = custom_toggle;
  sensitivity->text1 = text1;
  sensitivity->text2 = text2;
  sensitivity->preview = preview;
  sensitivity->tokens1 = GTK_WIDGET(tokens1);
  sensitivity->tokens2 = GTK_WIDGET(tokens2);
  g_object_set_data_full(G_OBJECT(type), "sensitivity", sensitivity, +[](gpointer data) { delete static_cast<NotificationSensitivity *>(data); });
  g_object_set_data(G_OBJECT(disable_duration), "type-row", type);
  g_object_set_data(G_OBJECT(custom_toggle), "type-row", type);
  ApplyNotificationSensitivity(ADW_COMBO_ROW(type));
  g_object_set_data(G_OBJECT(page), "osd-type-row", type);
  g_signal_connect(type, "notify::selected",
                   G_CALLBACK((+[](AdwComboRow *combo, GParamSpec *, gpointer) {
                     ApplyNotificationSensitivity(combo);
                     GtkWidget *page = gtk_widget_get_ancestor(GTK_WIDGET(combo), ADW_TYPE_PREFERENCES_PAGE);
                     UpdatePrettyPreview(page, combo);
                   })),
                   nullptr);
  g_signal_connect(page, "map",
                   G_CALLBACK((+[](GtkWidget *widget, gpointer data) { UpdatePrettyPreview(widget, ADW_COMBO_ROW(data)); })), type);
  g_signal_connect(page, "unmap",
                   G_CALLBACK((+[](GtkWidget *widget, gpointer data) { UpdatePrettyPreview(widget, ADW_COMBO_ROW(data)); })), type);
  g_signal_connect(disable_duration, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     if (auto *combo = ADW_COMBO_ROW(g_object_get_data(G_OBJECT(row), "type-row"))) {
                       ApplyNotificationSensitivity(combo);
                     }
                   }),
                   nullptr);
  g_signal_connect(custom_toggle, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer) {
                     if (auto *combo = ADW_COMBO_ROW(g_object_get_data(G_OBJECT(row), "type-row"))) {
                       ApplyNotificationSensitivity(combo);
                     }
                   }),
                   nullptr);
  return page;
}
