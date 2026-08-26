#include "ui/settingsdialog.h"

#include "config.h"
#include "core/application.h"
#include "core/settings.h"

#include <adwaita.h>

namespace {

void AddToggle(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *subtitle, bool fallback) {
  AdwSwitchRow *row = ADW_SWITCH_ROW(adw_switch_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
  if (subtitle) {
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle);
  }
  adw_switch_row_set_active(row, settings->BoolValue(key, fallback));
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "notify::active", G_CALLBACK(+[](AdwSwitchRow *switch_row, GParamSpec *, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(switch_row), "settings-key"));
                     s->SetBoolValue(settings_key, adw_switch_row_get_active(switch_row));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

void AddEntry(AdwPreferencesGroup *group, Settings *settings, const char *key, const char *title, const char *fallback = "") {
  AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
  adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), title);
  gtk_editable_set_text(GTK_EDITABLE(row), settings->Value(key, fallback).c_str());
  g_object_set_data_full(G_OBJECT(row), "settings-key", g_strdup(key), g_free);
  g_signal_connect(row, "changed", G_CALLBACK(+[](AdwEntryRow *entry, gpointer data) {
                     auto *s = static_cast<Settings *>(data);
                     const char *settings_key = static_cast<const char *>(g_object_get_data(G_OBJECT(entry), "settings-key"));
                     s->SetValue(settings_key, gtk_editable_get_text(GTK_EDITABLE(entry)));
                     s->Sync();
                   }),
                   settings);
  adw_preferences_group_add(group, GTK_WIDGET(row));
}

AdwPreferencesPage *MakePage(const char *name, const char *icon) {
  AdwPreferencesPage *page = ADW_PREFERENCES_PAGE(adw_preferences_page_new());
  adw_preferences_page_set_title(page, name);
  adw_preferences_page_set_icon_name(page, icon);
  return page;
}

}  // namespace

void SettingsDialog::Show(GtkWindow *parent, Application *app) {
  AdwPreferencesDialog *dialog = ADW_PREFERENCES_DIALOG(adw_preferences_dialog_new());
  auto *settings = new Settings();

  {
    settings->BeginGroup("Behaviour");
    AdwPreferencesPage *page = MakePage("Behaviour", "preferences-system-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Playback");
    AddToggle(group, settings, "resumeplayback", "Resume playback on startup", nullptr, true);
    AddToggle(group, settings, "stopplayifclosed", "Stop playing if the window is closed", nullptr, false);
    AddToggle(group, settings, "continueonerror", "Continue on error", nullptr, true);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Collection");
    AdwPreferencesPage *page = MakePage("Collection", "media-optical-cd-audio-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Library");
    AddToggle(group, settings, "startupscan", "Scan collection on startup", nullptr, true);
    AddToggle(group, settings, "monitor", "Watch folders for changes", nullptr, true);
    AddToggle(group, settings, "prettycovers", "Use pretty covers", nullptr, true);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Backend");
    AdwPreferencesPage *page = MakePage("Backend", "audio-card-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(group, "Output");
    AddEntry(group, settings, "output", "GStreamer output", "autoaudiosink");
    AddEntry(group, settings, "device", "Device");
    AddToggle(group, settings, "rgenabled", "ReplayGain / EBU R128", "Normalize volume using ReplayGain or EBU R128", false);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Playlist");
    AdwPreferencesPage *page = MakePage("Playlist", "view-list-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "alternating_row_colors", "Alternating row colors", nullptr, true);
    AddToggle(group, settings, "barcodes", "Show barcodes", nullptr, false);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Scrobbler");
    AdwPreferencesPage *page = MakePage("Scrobbler", "send-to-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "Last.fm", "Last.fm", nullptr, false);
    AddToggle(group, settings, "ListenBrainz", "ListenBrainz", nullptr, false);
#ifdef HAVE_SUBSONIC
    AddToggle(group, settings, "Subsonic", "Subsonic scrobble", nullptr, false);
#endif
    AddEntry(group, settings, "username", "Username");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Covers");
    AdwPreferencesPage *page = MakePage("Covers", "image-x-generic-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    for (CoverProvider *provider : app->cover_providers()->All()) {
      AddToggle(group, settings, provider->name().c_str(), provider->name().c_str(), nullptr, true);
    }
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Lyrics");
    AdwPreferencesPage *page = MakePage("Lyrics", "text-x-generic-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    for (LyricsProvider *provider : app->lyrics_providers()->All()) {
      AddToggle(group, settings, provider->name().c_str(), provider->name().c_str(), nullptr, true);
    }
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Transcoder");
    AdwPreferencesPage *page = MakePage("Transcoding", "document-save-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "format", "Default format", "flac");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("NetworkProxy");
    AdwPreferencesPage *page = MakePage("Proxy", "network-workgroup-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "type", "Type (none/manual)", "none");
    AddEntry(group, settings, "hostname", "Hostname");
    AddEntry(group, settings, "port", "Port", "0");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Appearance");
    AdwPreferencesPage *page = MakePage("Appearance", "applications-graphics-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "systemtheme", "Follow system theme", nullptr, true);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Context");
    AdwPreferencesPage *page = MakePage("Context", "view-paged-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "enable_album", "Show album", nullptr, true);
    AddToggle(group, settings, "enable_lyrics", "Show lyrics", nullptr, true);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("OSD");
    AdwPreferencesPage *page = MakePage("Notifications", "preferences-system-notifications-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "enabled", "Show notifications", nullptr, true);
    AddToggle(group, settings, "showart", "Show album art", nullptr, true);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("GlobalShortcuts");
    AdwPreferencesPage *page = MakePage("Shortcuts", "input-keyboard-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "enabled", "Enable global shortcuts", nullptr, true);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Moodbar");
    AdwPreferencesPage *page = MakePage("Moodbar", "weather-clear-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "enabled", "Show moodbar", nullptr, true);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Waveform");
    AdwPreferencesPage *page = MakePage("Waveform", "weather-showers-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "enabled", "Show waveform seek bar", nullptr, true);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
#ifdef HAVE_SUBSONIC
  {
    settings->BeginGroup("Subsonic");
    AdwPreferencesPage *page = MakePage("Subsonic", "network-server-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "url", "Server URL");
    AddEntry(group, settings, "username", "Username");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
#endif
#ifdef HAVE_TIDAL
  {
    settings->BeginGroup("Tidal");
    AdwPreferencesPage *page = MakePage("Tidal", "emblem-shared-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "clientid", "Client ID");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
#endif
#ifdef HAVE_QOBUZ
  {
    settings->BeginGroup("Qobuz");
    AdwPreferencesPage *page = MakePage("Qobuz", "emblem-shared-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "appid", "App ID");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
#endif
#ifdef HAVE_SPOTIFY
  {
    settings->BeginGroup("Spotify");
    AdwPreferencesPage *page = MakePage("Spotify", "emblem-shared-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "clientid", "Client ID");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
#endif
  {
    settings->BeginGroup("RadioBrowser");
    AdwPreferencesPage *page = MakePage("Radio", "network-wireless-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "server", "Radio Browser server", "https://de1.api.radio-browser.info");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }

  g_signal_connect(dialog, "closed", G_CALLBACK(+[](AdwDialog *, gpointer data) {
                     delete static_cast<Settings *>(data);
                   }),
                   settings);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
  (void)app;
}
