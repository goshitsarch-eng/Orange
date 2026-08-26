#include "ui/settingsdialog.h"

#include "config.h"
#include "core/application.h"
#include "core/oauthenticator.h"
#include "core/settings.h"
#include "ui/dialogs.h"
#include "utilities/jsonutils.h"

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
    AddToggle(group, settings, "showtrayicon", "Show system tray icon", nullptr, true);
    AddToggle(group, settings, "keeprunning", "Keep running in the tray when the window is closed", nullptr, false);
    AddToggle(group, settings, "starthidden", "Start hidden", nullptr, false);
    AddToggle(group, settings, "songtracking", "Track songs with Chromaprint / AcoustID", nullptr, false);
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
    AddToggle(group, settings, "variousartists", "Group various artists albums", nullptr, true);
    AddToggle(group, settings, "show_dividers", "Show artist / album dividers", nullptr, true);
    AdwPreferencesGroup *dirs = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(dirs, "Folders");
    for (const CollectionDirectory &directory : app->collection()->backend()->Directories()) {
      AdwActionRow *row = ADW_ACTION_ROW(adw_action_row_new());
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), directory.path.c_str());
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), directory.subdirs ? "Including subfolders" : "This folder only");
      adw_preferences_group_add(dirs, GTK_WIDGET(row));
    }
    adw_preferences_page_add(page, group);
    adw_preferences_page_add(page, dirs);
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
    AddEntry(group, settings, "rgmode", "ReplayGain mode (album/track)", "album");
    AddEntry(group, settings, "rgpreamp", "ReplayGain preamp (dB)", "0");
    AddEntry(group, settings, "stereobalance", "Stereo balance (-100..100)", "0");
    AddToggle(group, settings, "fading", "Cross-fade between tracks", nullptr, false);
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Playlist");
    AdwPreferencesPage *page = MakePage("Playlist", "view-list-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "alternating_row_colors", "Alternating row colors", nullptr, true);
    AddToggle(group, settings, "barcodes", "Show barcodes", nullptr, false);
    AddToggle(group, settings, "greyout_unavailable", "Grey out unavailable songs", nullptr, true);
    AddToggle(group, settings, "warnclose", "Warn when closing a playlist", nullptr, true);
    AddEntry(group, settings, "columns", "Visible columns",
             "Track,Title,Artist,Album,Album artist,Length,Year,Genre,Bitrate,Sample rate,Plays,Rating,Filename");
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
    AdwActionRow *login = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(login), "Account");
    GtkWidget *button = gtk_button_new_with_label("Sign in");
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       Dialogs::Login(nullptr, "Last.fm", [application](const std::string &user, const std::string &pass) {
                         for (ScrobblerService *service : application->scrobbler()->All()) {
                           service->Authenticate(user, pass);
                         }
                       });
                     }),
                     app);
    adw_action_row_add_suffix(login, button);
    adw_preferences_group_add(group, GTK_WIDGET(login));
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Covers");
    AdwPreferencesPage *page = MakePage("Covers", "image-x-generic-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "save_dest", "Save destination (album/embedded/cache)", "album");
    AddEntry(group, settings, "filename", "Cover filename", "cover.jpg");
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
    AddEntry(group, settings, "type", "Type (native/pretty/both)", "native");
    AddEntry(group, settings, "timeout", "Pretty OSD timeout (ms)", "4000");
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("GlobalShortcuts");
    AdwPreferencesPage *page = MakePage("Shortcuts", "input-keyboard-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "enabled", "Enable global shortcuts", nullptr, true);
    AddEntry(group, settings, "playpause", "Play / Pause", "MediaPlay");
    AddEntry(group, settings, "next", "Next", "MediaNext");
    AddEntry(group, settings, "previous", "Previous", "MediaPrevious");
    AdwActionRow *grab = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(grab), "Grab play/pause shortcut");
    GtkWidget *grab_button = gtk_button_new_with_label("Grab");
    g_signal_connect(grab_button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                       auto *s = static_cast<Settings *>(data);
                       Dialogs::GrabShortcut(nullptr, [s](const std::string &accel) {
                         s->BeginGroup("GlobalShortcuts");
                         s->SetValue("playpause", accel);
                         s->Sync();
                       });
                     }),
                     settings);
    adw_action_row_add_suffix(grab, grab_button);
    adw_preferences_group_add(group, GTK_WIDGET(grab));
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
  {
    settings->BeginGroup("Moodbar");
    AdwPreferencesPage *page = MakePage("Moodbar", "weather-clear-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddToggle(group, settings, "enabled", "Show moodbar", nullptr, true);
    AddEntry(group, settings, "style", "Style (normal/angry/frozen/happy)", "normal");
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
    AdwActionRow *login = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(login), "Server login");
    GtkWidget *button = gtk_button_new_with_label("Sign in");
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       Dialogs::Login(nullptr, "Subsonic", [application](const std::string &user, const std::string &token) {
                         if (StreamingService *service = application->streaming_services()->ServiceByName("Subsonic")) {
                           service->Login(user, token);
                         }
                       });
                     }),
                     app);
    adw_action_row_add_suffix(login, button);
    adw_preferences_group_add(group, GTK_WIDGET(login));
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
    AddEntry(group, settings, "clientsecret", "Client secret");
    AdwActionRow *login = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(login), "OAuth");
    GtkWidget *button = gtk_button_new_with_label("Sign in");
    g_signal_connect(button, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       Settings s;
                       s.BeginGroup("Tidal");
                       const std::string client_id = s.Value("clientid");
                       if (client_id.empty()) {
                         Dialogs::Login(nullptr, "Tidal", [application](const std::string &user, const std::string &token) {
                           if (StreamingService *service = application->streaming_services()->ServiceByName("Tidal")) {
                             service->Login(user, token);
                           }
                         });
                         return;
                       }
                       auto *oauth = new OAuthenticator(application->network());
                       oauth->AuthorizeInBrowser("https://login.tidal.com/authorize", client_id, "r_usr r_res w_usr",
                                                 [application, oauth](const std::string &code, const std::string &error) {
                                                   if (!code.empty()) {
                                                     Settings ts;
                                                     ts.BeginGroup("Tidal");
                                                     oauth->ExchangeCode("https://auth.tidal.com/v1/oauth2/token", ts.Value("clientid"),
                                                                         ts.Value("clientsecret"), code,
                                                                         [application, oauth](const std::string &body, const std::string &) {
                                                                           const std::string token = JsonUtils::GetString(body, {"access_token"});
                                                                           if (StreamingService *service = application->streaming_services()->ServiceByName("Tidal")) {
                                                                             service->Login({}, token.empty() ? body : token);
                                                                           }
                                                                           delete oauth;
                                                                         });
                                                   } else {
                                                     (void)error;
                                                     delete oauth;
                                                   }
                                                 });
                     })),
                     app);
    adw_action_row_add_suffix(login, button);
    adw_preferences_group_add(group, GTK_WIDGET(login));
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
    AdwActionRow *login = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(login), "Login");
    GtkWidget *button = gtk_button_new_with_label("Sign in");
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       Dialogs::Login(nullptr, "Qobuz", [application](const std::string &user, const std::string &token) {
                         if (StreamingService *service = application->streaming_services()->ServiceByName("Qobuz")) {
                           service->Login(user, token);
                         }
                       });
                     }),
                     app);
    adw_action_row_add_suffix(login, button);
    adw_preferences_group_add(group, GTK_WIDGET(login));
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
    AddEntry(group, settings, "clientsecret", "Client secret");
    AdwActionRow *login = ADW_ACTION_ROW(adw_action_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(login), "OAuth");
    GtkWidget *button = gtk_button_new_with_label("Sign in");
    g_signal_connect(button, "clicked", G_CALLBACK((+[](GtkButton *, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       Settings s;
                       s.BeginGroup("Spotify");
                       const std::string client_id = s.Value("clientid");
                       if (client_id.empty()) {
                         Dialogs::Login(nullptr, "Spotify", [application](const std::string &user, const std::string &token) {
                           if (StreamingService *service = application->streaming_services()->ServiceByName("Spotify")) {
                             service->Login(user, token);
                           }
                         });
                         return;
                       }
                       auto *oauth = new OAuthenticator(application->network());
                       oauth->AuthorizeInBrowser("https://accounts.spotify.com/authorize", client_id, "user-read-private user-read-email",
                                                 [application, oauth](const std::string &code, const std::string &error) {
                                                   if (code.empty()) {
                                                     (void)error;
                                                     delete oauth;
                                                     return;
                                                   }
                                                   Settings ss;
                                                   ss.BeginGroup("Spotify");
                                                   oauth->ExchangeCode("https://accounts.spotify.com/api/token", ss.Value("clientid"), ss.Value("clientsecret"),
                                                                       code, [application, oauth](const std::string &body, const std::string &) {
                                                                         const std::string token = JsonUtils::GetString(body, {"access_token"});
                                                                         if (StreamingService *service = application->streaming_services()->ServiceByName("Spotify")) {
                                                                           service->Login({}, token.empty() ? body : token);
                                                                         }
                                                                         delete oauth;
                                                                       });
                                                 });
                     })),
                     app);
    adw_action_row_add_suffix(login, button);
    adw_preferences_group_add(group, GTK_WIDGET(login));
    adw_preferences_page_add(page, group);
    adw_preferences_dialog_add(dialog, page);
  }
#endif
  {
    settings->BeginGroup("RadioBrowser");
    AdwPreferencesPage *page = MakePage("Radio", "network-wireless-symbolic");
    AdwPreferencesGroup *group = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    AddEntry(group, settings, "server", "Radio Browser server", "https://de1.api.radio-browser.info");
    AddEntry(group, settings, "country", "Country filter");
    AddToggle(group, settings, "hidebroken", "Hide broken stations", nullptr, true);
    adw_preferences_page_add(page, group);
    settings->BeginGroup("SomaFM");
    AdwPreferencesGroup *soma = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(soma, "SomaFM");
    AddEntry(soma, settings, "quality", "Quality (64/128/256)", "128");
    adw_preferences_page_add(page, soma);
    settings->BeginGroup("RadioParadise");
    AdwPreferencesGroup *rp = ADW_PREFERENCES_GROUP(adw_preferences_group_new());
    adw_preferences_group_set_title(rp, "Radio Paradise");
    AddEntry(rp, settings, "quality", "Stream (aac-320/aac-128/mp3-192)", "aac-320");
    adw_preferences_page_add(page, rp);
    adw_preferences_dialog_add(dialog, page);
  }

  g_signal_connect(dialog, "closed", G_CALLBACK(+[](AdwDialog *, gpointer data) {
                     delete static_cast<Settings *>(data);
                   }),
                   settings);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
  (void)app;
}
