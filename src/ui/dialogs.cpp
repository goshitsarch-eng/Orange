#include "ui/dialogs.h"

#include "core/application.h"
#include "equalizer/equalizer.h"
#include "organize/organize.h"
#include "transcoder/transcoder.h"

#include <adwaita.h>

void Dialogs::AddStream(GtkWindow *parent, const std::function<void(const std::string &, const std::string &)> &callback) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Add stream", "Enter a name and stream URL."));
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  GtkWidget *name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Name");
  GtkWidget *url = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(url), "https://");
  gtk_box_append(GTK_BOX(box), name);
  gtk_box_append(GTK_BOX(box), url);
  adw_alert_dialog_set_extra_child(dialog, box);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "add", "Add", nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "add", ADW_RESPONSE_SUGGESTED);
  auto *cb = new std::function<void(const std::string &, const std::string &)>(callback);
  g_object_set_data(G_OBJECT(dialog), "name-entry", name);
  g_object_set_data(G_OBJECT(dialog), "url-entry", url);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     auto *fn = static_cast<std::function<void(const std::string &, const std::string &)> *>(data);
                     if (g_strcmp0(response, "add") == 0) {
                       GtkWidget *name_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(alert), "name-entry"));
                       GtkWidget *url_entry = GTK_WIDGET(g_object_get_data(G_OBJECT(alert), "url-entry"));
                       (*fn)(gtk_editable_get_text(GTK_EDITABLE(name_entry)), gtk_editable_get_text(GTK_EDITABLE(url_entry)));
                     }
                     delete fn;
                   }),
                   cb);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

void Dialogs::CoverManager(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Cover manager");
  GtkWidget *status = adw_status_page_new();
  adw_status_page_set_icon_name(ADW_STATUS_PAGE(status), "image-x-generic-symbolic");
  adw_status_page_set_title(ADW_STATUS_PAGE(status), "Album covers");
  adw_status_page_set_description(ADW_STATUS_PAGE(status),
                                  "Search Last.fm, MusicBrainz, Discogs, Musixmatch, Deezer, Tidal, Qobuz and Spotify for missing artwork.");
  GtkWidget *button = gtk_button_new_with_label("Fetch cover for current song");
  g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     application->cover_providers()->Fetch(application->player()->current_song(), [](const std::string &, const std::string &) {});
                   }),
                   app);
  adw_status_page_set_child(ADW_STATUS_PAGE(status), button);
  adw_dialog_set_child(dialog, status);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::Equalizer(GtkWindow *parent, class Equalizer *equalizer) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Equalizer");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *enable = gtk_check_button_new_with_label("Enable equalizer");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(enable), equalizer->enabled());
  g_signal_connect(enable, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer data) {
                     static_cast<class Equalizer *>(data)->set_enabled(gtk_check_button_get_active(button));
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(box), enable);
  GtkWidget *bands = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  for (int i = 0; i < 10; ++i) {
    GtkWidget *scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, -12, 12, 1);
    gtk_range_set_inverted(GTK_RANGE(scale), TRUE);
    gtk_range_set_value(GTK_RANGE(scale), equalizer->gains()[static_cast<size_t>(i)]);
    gtk_widget_set_size_request(scale, -1, 160);
    g_object_set_data(G_OBJECT(scale), "band", GINT_TO_POINTER(i));
    g_signal_connect(scale, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                       const int band = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(range), "band"));
                       static_cast<class Equalizer *>(data)->set_gain(band, static_cast<int>(gtk_range_get_value(range)));
                     }),
                     equalizer);
    gtk_box_append(GTK_BOX(bands), scale);
  }
  gtk_box_append(GTK_BOX(box), bands);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::Transcode(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Transcode");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *formats = gtk_drop_down_new_from_strings(
      (const char *[]){"MP3", "AAC", "FLAC", "Ogg Vorbis", "Opus", "Speex", "WavPack", "ASF", nullptr});
  gtk_box_append(GTK_BOX(box), formats);
  GtkWidget *start = gtk_button_new_with_label("Start");
  g_signal_connect(start, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     if (application->playlist_manager()->active()) {
                       for (const Song &song : application->playlist_manager()->active()->songs()) {
                         application->transcoder()->AddJob(song, song.basefilename() + ".flac", Transcoder::Format::FLAC);
                       }
                       application->transcoder()->Start();
                     }
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), start);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::Organize(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Organize files");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  GtkWidget *entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), "%albumartist/%album/{%track - }%title");
  gtk_box_append(GTK_BOX(box), entry);
  (void)app;
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::TagFetcher(GtkWindow *parent, Application *app) {
  app->tag_fetcher()->Fetch(app->player()->current_song());
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Fetch tags", "Searching AcoustID and MusicBrainz for the current track."));
  adw_alert_dialog_add_response(dialog, "ok", "OK");
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

void Dialogs::EditTag(GtkWindow *parent, Application *app) {
  Song song = app->player()->current_song();
  if (app->playlist_manager()->active() && app->playlist_manager()->current_row() >= 0) {
    song = app->playlist_manager()->current_song();
  }
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Edit tags");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  auto add = [&](const char *label, const std::string &value) {
    AdwEntryRow *row = ADW_ENTRY_ROW(adw_entry_row_new());
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), label);
    gtk_editable_set_text(GTK_EDITABLE(row), value.c_str());
    gtk_box_append(GTK_BOX(box), GTK_WIDGET(row));
  };
  add("Title", song.title());
  add("Artist", song.artist());
  add("Album", song.album());
  add("Album artist", song.albumartist());
  add("Composer", song.composer());
  add("Genre", song.genre());
  add("Comment", song.comment());
  add("Lyrics", song.lyrics());
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::Shortcuts(GtkWindow *parent) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Keyboard shortcuts");
  GtkWidget *label = gtk_label_new("Space  Play/Pause\nCtrl+Right  Next\nCtrl+Left  Previous\nCtrl+Up  Volume up\nCtrl+Down  Volume down\nCtrl+Q  Quit\nCtrl+,  Preferences");
  gtk_widget_set_margin_start(label, 24);
  gtk_widget_set_margin_end(label, 24);
  gtk_widget_set_margin_top(label, 24);
  gtk_widget_set_margin_bottom(label, 24);
  adw_dialog_set_child(dialog, label);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::Error(GtkWindow *parent, const std::string &message) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Error", message.c_str()));
  adw_alert_dialog_add_response(dialog, "ok", "OK");
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
