#include "ui/settingsdialog.h"

#include "config.h"
#include "core/application.h"
#include "core/settings.h"
#include "settings/analyzersettingspage.h"
#include "settings/appearancesettingspage.h"
#include "settings/backendsettingspage.h"
#include "settings/behavioursettingspage.h"
#include "settings/collectionsettingspage.h"
#include "settings/contextsettingspage.h"
#include "settings/coverssettingspage.h"
#include "settings/globalshortcutssettingspage.h"
#include "settings/lyricssettingspage.h"
#include "settings/moodbarsettingspage.h"
#include "settings/networkproxysettingspage.h"
#include "settings/notificationssettingspage.h"
#include "settings/playlistsettingspage.h"
#include "settings/qobuzsettingspage.h"
#include "settings/radiosettingspage.h"
#include "settings/scrobblersettingspage.h"
#include "settings/spotifysettingspage.h"
#include "settings/subsonicsettingspage.h"
#include "settings/tidalsettingspage.h"
#include "settings/transcodersettingspage.h"
#include "settings/waveformsettingspage.h"
#include "translations/translations.h"

#include <adwaita.h>

void SettingsDialog::Show(GtkWindow *parent, Application *app, const std::function<void()> &closed, const char *page_name) {
  AdwPreferencesDialog *dialog = ADW_PREFERENCES_DIALOG(adw_preferences_dialog_new());
  adw_dialog_set_title(ADW_DIALOG(dialog), Translations::CStr("Preferences"));
  auto *settings = new Settings();
  auto *on_closed = new std::function<void()>(closed);

  adw_preferences_dialog_add(dialog, BehaviourSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, CollectionSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, BackendSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, PlaylistSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, ScrobblerSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, CoversSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, LyricsSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, TranscoderSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, NetworkProxySettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, AppearanceSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, ContextSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, NotificationsSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, GlobalShortcutsSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, MoodbarSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, WaveformSettingsPage::Create(settings, app));
  adw_preferences_dialog_add(dialog, AnalyzerSettingsPage::Create(settings, app));
#ifdef HAVE_SUBSONIC
  adw_preferences_dialog_add(dialog, SubsonicSettingsPage::Create(settings, app));
#endif
#ifdef HAVE_TIDAL
  adw_preferences_dialog_add(dialog, TidalSettingsPage::Create(settings, app));
#endif
#ifdef HAVE_QOBUZ
  adw_preferences_dialog_add(dialog, QobuzSettingsPage::Create(settings, app));
#endif
#ifdef HAVE_SPOTIFY
  adw_preferences_dialog_add(dialog, SpotifySettingsPage::Create(settings, app));
#endif
  adw_preferences_dialog_add(dialog, RadioSettingsPage::Create(settings, app));

  if (page_name && page_name[0] != '\0') {
    adw_preferences_dialog_set_visible_page_name(dialog, page_name);
  }

  g_object_set_data_full(G_OBJECT(dialog), "settings", settings, [](gpointer p) { delete static_cast<Settings *>(p); });
  g_object_set_data_full(G_OBJECT(dialog), "closed", on_closed, [](gpointer p) { delete static_cast<std::function<void()> *>(p); });
  g_signal_connect(dialog, "closed", G_CALLBACK(+[](AdwDialog *alert, gpointer) {
                     if (auto *fn = static_cast<std::function<void()> *>(g_object_get_data(G_OBJECT(alert), "closed"))) {
                       if (*fn) {
                         (*fn)();
                       }
                     }
                   }),
                   nullptr);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
