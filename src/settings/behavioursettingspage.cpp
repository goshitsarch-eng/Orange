#include "settings/behavioursettingspage.h"

#include "constants/behavioursettings.h"
#include "core/application.h"
#include "settings/behavioursettingslabels.h"
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
  AdwPreferencesPage *page = SettingsPage::MakePage(BehaviourSettingsLabels::PageTitle(), "preferences-system-symbolic");
  const bool tray_available = app && app->tray() ? app->tray()->available() : true;
  bool show_tray = settings->BoolValue(BehaviourSettings::kShowTrayIcon, BehaviourSettings::kDefaultShowTrayIcon);
  if (!tray_available) {
    show_tray = false;
  }
  AdwPreferencesGroup *startup = SettingsPage::AddGroup(page, BehaviourSettingsLabels::OnStartup());
  SettingsPage::AddToggle(startup, settings, BehaviourSettings::kResumePlayback, BehaviourSettingsLabels::ResumePlayback(), nullptr,
                          BehaviourSettings::kDefaultResumePlayback);
  SettingsPage::AddCombo(startup, settings, BehaviourSettings::kStartupBehaviour, BehaviourSettingsLabels::OnStartup(),
                         BehaviourStartupChoices::StartupChoices(tray_available, show_tray),
                         BehaviourStartupChoices::EffectiveStartup(
                             settings->Value(BehaviourSettings::kStartupBehaviour,
                                             std::to_string(static_cast<int>(BehaviourSettings::kDefaultStartupBehaviour))),
                             tray_available, show_tray));
  SettingsPage::AddCombo(startup, settings, nullptr, BehaviourSettingsLabels::Language(), LanguageChoices::All(),
                         settings->Value(BehaviourSettings::kLanguage), [settings](const std::string &id) {
                           settings->BeginGroup(BehaviourSettings::kSettingsGroup);
                           settings->SetValue(BehaviourSettings::kLanguage, id);
                           settings->Sync();
                         });
  GtkWidget *language_note = gtk_label_new(Translations::CStr(BehaviourSettingsLabels::LanguageRestart()));
  gtk_widget_add_css_class(language_note, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(language_note), TRUE);
  gtk_label_set_xalign(GTK_LABEL(language_note), 0.0f);
  gtk_widget_set_margin_start(language_note, 12);
  gtk_widget_set_margin_end(language_note, 12);
  gtk_widget_set_margin_bottom(language_note, 8);
  adw_preferences_group_add(startup, language_note);

  AdwPreferencesGroup *tray = SettingsPage::AddGroup(page, "System tray");
  GtkWidget *show_tray_row = SettingsPage::AddToggle(tray, settings, BehaviourSettings::kShowTrayIcon, BehaviourSettingsLabels::ShowTray(),
                                                    nullptr, BehaviourSettings::kDefaultShowTrayIcon);
  gtk_widget_set_sensitive(show_tray_row, tray_available ? TRUE : FALSE);
  GtkWidget *keep_running = SettingsPage::AddToggle(tray, settings, BehaviourSettings::kKeepRunning, BehaviourSettingsLabels::KeepRunning(),
                                                    nullptr, BehaviourSettings::kDefaultKeepRunning);
  GtkWidget *tray_progress =
      SettingsPage::AddToggle(tray, settings, BehaviourSettings::kTrayIconProgress, BehaviourSettingsLabels::TrayProgress(), nullptr,
                              BehaviourSettings::kDefaultTrayIconProgress);
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
  SettingsPage::AddToggle(tray, settings, BehaviourSettings::kTaskbarProgress, BehaviourSettingsLabels::TaskbarProgress(), nullptr,
                          BehaviourSettings::kDefaultTaskbarProgress);
  SettingsPage::AddToggle(tray, settings, BehaviourSettings::kPlayingWidget, BehaviourSettingsLabels::PlayingWidget(), nullptr,
                          BehaviourSettings::kDefaultPlayingWidget);

  AdwPreferencesGroup *playback = SettingsPage::AddGroup(page, "Playback");
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kMenuPlayMode,
                            BehaviourSettingsLabels::MenuPlay(), BehaviourSettingsLabels::PlayChoices(),
                            static_cast<int>(BehaviourSettings::kDefaultMenuPlayMode));
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kMenuPreviousMode,
                            BehaviourSettingsLabels::PreviousMode(), BehaviourSettingsLabels::PreviousChoices(),
                            static_cast<int>(BehaviourSettings::kDefaultMenuPreviousMode));
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kDoubleClickAddMode,
                            BehaviourSettingsLabels::DoubleClickAdd(), BehaviourSettingsLabels::DoubleClickAddChoices(),
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickAddMode));
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kDoubleClickPlayMode,
                            BehaviourSettingsLabels::DoubleClickPlay(), BehaviourSettingsLabels::PlayChoices(),
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickPlayMode));
  SettingsPage::AddIntCombo(playback, settings, BehaviourSettings::kSettingsGroup, BehaviourSettings::kDoubleClickPlaylistAddMode,
                            BehaviourSettingsLabels::DoubleClickPlaylist(), BehaviourSettingsLabels::DoubleClickPlaylistChoices(),
                            static_cast<int>(BehaviourSettings::kDefaultDoubleClickPlaylistAddMode));
  SettingsPage::AddDescription(playback, BehaviourSettingsLabels::Seeking());
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kSeekStepSec, BehaviourSettingsLabels::TimeStep(),
                            BehaviourSettings::kDefaultSeekStepSec);
  SettingsPage::AddIntEntry(playback, settings, BehaviourSettings::kVolumeIncrement, BehaviourSettingsLabels::VolumeIncrement(),
                            static_cast<int>(BehaviourSettings::kDefaultVolumeIncrement));
  return page;
}
