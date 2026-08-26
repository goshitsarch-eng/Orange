#include "settings/behavioursettingspage.h"

#include "constants/behavioursettings.h"
#include "core/application.h"
#include "settings/behaviourstartupchoices.h"
#include "settings/settingspage.h"
#include "systemtrayicon/systemtrayicon.h"
#include "translations/languagechoices.h"
#include "translations/translations.h"

namespace {

struct TrayWidgets {
  bool available = true;
  GtkWidget *keep = nullptr;
  GtkWidget *progress = nullptr;
};

}  // namespace

AdwPreferencesPage *BehaviourSettingsPage::Create(Settings *settings, Application *app) {
  settings->BeginGroup(BehaviourSettings::kSettingsGroup);
  AdwPreferencesPage *page = SettingsPage::MakePage("Behaviour", "preferences-system-symbolic");
  const bool tray_available = app && app->tray() ? app->tray()->available() : true;
  bool show_tray = settings->BoolValue(BehaviourSettings::kShowTrayIcon, BehaviourSettings::kDefaultShowTrayIcon);
  if (!tray_available) {
    show_tray = false;
  }
  AdwPreferencesGroup *startup = SettingsPage::AddGroup(page, "Startup");
  SettingsPage::AddToggle(startup, settings, BehaviourSettings::kResumePlayback, "Resume playback on startup", nullptr,
                          BehaviourSettings::kDefaultResumePlayback);
  SettingsPage::AddCombo(startup, settings, BehaviourSettings::kStartupBehaviour, "Startup behaviour",
                         BehaviourStartupChoices::StartupChoices(tray_available, show_tray),
                         BehaviourStartupChoices::EffectiveStartup(
                             settings->Value(BehaviourSettings::kStartupBehaviour,
                                             std::to_string(static_cast<int>(BehaviourSettings::kDefaultStartupBehaviour))),
                             tray_available, show_tray));
  SettingsPage::AddCombo(startup, settings, nullptr, "Language", LanguageChoices::All(), settings->Value(BehaviourSettings::kLanguage),
                         [settings](const std::string &id) {
                           settings->BeginGroup(BehaviourSettings::kSettingsGroup);
                           settings->SetValue(BehaviourSettings::kLanguage, id);
                           settings->Sync();
                         });
  GtkWidget *language_note = gtk_label_new(Translations::CStr("You will need to restart Strawberry if you change the language."));
  gtk_widget_add_css_class(language_note, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(language_note), TRUE);
  gtk_label_set_xalign(GTK_LABEL(language_note), 0.0f);
  gtk_widget_set_margin_start(language_note, 12);
  gtk_widget_set_margin_end(language_note, 12);
  gtk_widget_set_margin_bottom(language_note, 8);
  adw_preferences_group_add(startup, language_note);

  AdwPreferencesGroup *tray = SettingsPage::AddGroup(page, "System tray");
  GtkWidget *show_tray_row =
      SettingsPage::AddToggle(tray, settings, BehaviourSettings::kShowTrayIcon, "Show system tray icon", nullptr, BehaviourSettings::kDefaultShowTrayIcon);
  gtk_widget_set_sensitive(show_tray_row, tray_available ? TRUE : FALSE);
  GtkWidget *keep_running = SettingsPage::AddToggle(tray, settings, BehaviourSettings::kKeepRunning,
                                                   "Keep running in the tray when the window is closed", nullptr,
                                                   BehaviourSettings::kDefaultKeepRunning);
  GtkWidget *tray_progress = SettingsPage::AddToggle(tray, settings, BehaviourSettings::kTrayIconProgress, "Show progress on the tray icon",
                                                    nullptr, BehaviourSettings::kDefaultTrayIconProgress);
  const bool tray_sensitive = BehaviourStartupChoices::TrayDependentSensitive(tray_available, show_tray);
  gtk_widget_set_sensitive(keep_running, tray_sensitive ? TRUE : FALSE);
  gtk_widget_set_sensitive(tray_progress, tray_sensitive ? TRUE : FALSE);
  auto *tray_widgets = new TrayWidgets{tray_available, keep_running, tray_progress};
  g_object_set_data_full(G_OBJECT(page), "tray-widgets", tray_widgets, [](gpointer p) { delete static_cast<TrayWidgets *>(p); });
  g_signal_connect(show_tray_row, "notify::active", G_CALLBACK(+[](AdwSwitchRow *row, GParamSpec *, gpointer data) {
                     auto *state = static_cast<TrayWidgets *>(data);
                     const bool sensitive = BehaviourStartupChoices::TrayDependentSensitive(state->available, adw_switch_row_get_active(row));
                     gtk_widget_set_sensitive(state->keep, sensitive ? TRUE : FALSE);
                     gtk_widget_set_sensitive(state->progress, sensitive ? TRUE : FALSE);
                   }),
                   tray_widgets);
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
