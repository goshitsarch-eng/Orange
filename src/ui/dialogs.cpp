#include "ui/dialogs.h"

#include "core/application.h"
#include "core/settings.h"
#include "equalizer/equalizer.h"
#include "organize/organize.h"
#include "smartplaylists/smartplaylist.h"
#include "transcoder/transcoder.h"
#include "utilities/fileutils.h"
#include "utilities/timeutils.h"

#include <adwaita.h>

#include <cstdlib>
#include <vector>

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
  adw_dialog_set_content_width(dialog, 520);
  adw_dialog_set_content_height(dialog, 560);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  GtkWidget *status = gtk_label_new("Search Last.fm, MusicBrainz, Discogs, Musixmatch, Deezer, Tidal, Qobuz and Spotify for missing artwork.");
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  gtk_box_append(GTK_BOX(box), status);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  std::string last;
  for (const Song &song : app->collection()->Songs()) {
    const std::string album = song.EffectiveAlbumartist() + " – " + song.album();
    if (album == last || song.album().empty()) {
      continue;
    }
    last = album;
    auto *copy = new Song(song);
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), song.album().c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), song.EffectiveAlbumartist().c_str());
    GtkWidget *button = gtk_button_new_with_label("Fetch");
    gtk_widget_add_css_class(button, "flat");
    g_object_set_data_full(G_OBJECT(button), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
    g_signal_connect(button, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(btn), "song"));
                       if (!song) {
                         return;
                       }
                       application->cover_providers()->Fetch(*song, [btn](const std::string &image, const std::string &error) {
                         gtk_button_set_label(GTK_BUTTON(btn), image.empty() ? (error.empty() ? "Missing" : "Failed") : "Saved");
                       });
                     }),
                     app);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), button);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
  }
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  gtk_box_append(GTK_BOX(box), scroll);
  GtkWidget *current = gtk_button_new_with_label("Fetch cover for current song");
  g_signal_connect(current, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     application->cover_providers()->Fetch(application->player()->current_song(), [](const std::string &, const std::string &) {});
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), current);
  adw_dialog_set_child(dialog, box);
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
  const auto presets = equalizer->Presets();
  std::vector<const char *> names;
  for (const std::string &name : presets) {
    names.push_back(name.c_str());
  }
  names.push_back(nullptr);
  GtkWidget *preset = gtk_drop_down_new_from_strings(names.data());
  g_signal_connect(preset, "notify::selected", G_CALLBACK(+[](GtkDropDown *drop, GParamSpec *, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkStringObject *item = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(drop));
                     if (item) {
                       eq->LoadPreset(gtk_string_object_get_string(item));
                     }
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(box), preset);
  GtkWidget *preamp = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, -12, 12, 1);
  gtk_range_set_value(GTK_RANGE(preamp), equalizer->preamp());
  gtk_widget_set_tooltip_text(preamp, "Preamp");
  g_signal_connect(preamp, "value-changed", G_CALLBACK(+[](GtkRange *range, gpointer data) {
                     static_cast<class Equalizer *>(data)->set_preamp(static_cast<int>(gtk_range_get_value(range)));
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(box), preamp);
  GtkWidget *bands = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  static const char *kHz[] = {"60", "170", "310", "600", "1k", "3k", "6k", "12k", "14k", "16k"};
  for (int i = 0; i < 10; ++i) {
    GtkWidget *col = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
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
    gtk_box_append(GTK_BOX(col), scale);
    gtk_box_append(GTK_BOX(col), gtk_label_new(kHz[i]));
    gtk_box_append(GTK_BOX(bands), col);
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
  static const char *format_names[] = {"MP3", "AAC", "FLAC", "Ogg Vorbis", "Opus", "Speex", "WavPack", "ASF", nullptr};
  GtkWidget *formats = gtk_drop_down_new_from_strings(format_names);
  GtkWidget *quality = gtk_spin_button_new_with_range(0, 10, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(quality), 5);
  GtkWidget *dest = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(dest), g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) ? g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) : g_get_home_dir());
  gtk_box_append(GTK_BOX(box), gtk_label_new("Output format"));
  gtk_box_append(GTK_BOX(box), formats);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Quality"));
  gtk_box_append(GTK_BOX(box), quality);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Destination folder"));
  gtk_box_append(GTK_BOX(box), dest);
  GtkWidget *log = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(log), TRUE);
  gtk_box_append(GTK_BOX(box), log);
  GtkWidget *start = gtk_button_new_with_label("Start");
  g_object_set_data(G_OBJECT(start), "formats", formats);
  g_object_set_data(G_OBJECT(start), "dest", dest);
  g_object_set_data(G_OBJECT(start), "log", log);
  g_signal_connect(start, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *formats_w = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "formats"));
                     auto *dest_w = GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest"));
                     auto *log_w = GTK_LABEL(g_object_get_data(G_OBJECT(button), "log"));
                     const auto format = static_cast<Transcoder::Format>(gtk_drop_down_get_selected(formats_w));
                     const std::string dest_dir = gtk_editable_get_text(dest_w);
                     application->transcoder()->Cancel();
                     int count = 0;
                     if (application->playlist_manager()->active()) {
                       for (const Song &song : application->playlist_manager()->active()->songs()) {
                         const std::string name = FileUtils::BaseName(FileUtils::PathFromUri(song.url()));
                         const auto dot = name.rfind('.');
                         const std::string stem = dot == std::string::npos ? name : name.substr(0, dot);
                         application->transcoder()->AddJob(song, FileUtils::Join(dest_dir, stem + "." + Transcoder::Extension(format)), format);
                         ++count;
                       }
                       application->transcoder()->Start();
                     }
                     gtk_label_set_text(log_w, (std::to_string(count) + " jobs queued as " + Transcoder::FormatName(format)).c_str());
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
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *entry = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(entry), "%albumartist/%album/{%track - }%title");
  GtkWidget *dest = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(dest), g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) ? g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) : g_get_home_dir());
  GtkWidget *move = gtk_check_button_new_with_label("Move files instead of copying");
  GtkWidget *status = gtk_label_new("Uses the current playlist as the source.");
  GtkWidget *run = gtk_button_new_with_label("Organize");
  gtk_widget_add_css_class(run, "suggested-action");
  g_object_set_data(G_OBJECT(run), "format", entry);
  g_object_set_data(G_OBJECT(run), "dest", dest);
  g_object_set_data(G_OBJECT(run), "move", move);
  g_object_set_data(G_OBJECT(run), "status", status);
  g_signal_connect(run, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     if (!application->playlist_manager()->active()) {
                       return;
                     }
                     OrganizeFormat format(gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "format"))));
                     const std::string dest_dir = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest")));
                     const bool move_files = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "move")));
                     class Organize organize;
                     organize.Copy(application->playlist_manager()->active()->songs(), dest_dir, format, move_files);
                     gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "status")), "Organize finished.");
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Filename format"));
  gtk_box_append(GTK_BOX(box), entry);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Destination"));
  gtk_box_append(GTK_BOX(box), dest);
  gtk_box_append(GTK_BOX(box), move);
  gtk_box_append(GTK_BOX(box), run);
  gtk_box_append(GTK_BOX(box), status);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::TagFetcher(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Fetch tags");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *status = gtk_label_new("Searching AcoustID and MusicBrainz…");
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  gtk_box_append(GTK_BOX(box), status);
  gtk_box_append(GTK_BOX(box), list);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
  app->tag_fetcher()->Results.Connect([status, list](const SongList &songs) {
    gtk_label_set_text(GTK_LABEL(status), songs.empty() ? "No matches" : (std::to_string(songs.size()) + " matches").c_str());
    for (const Song &song : songs) {
      GtkWidget *row = adw_action_row_new();
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), song.PrettyTitle().c_str());
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), (song.artist() + " – " + song.album()).c_str());
      gtk_list_box_append(GTK_LIST_BOX(list), row);
    }
  });
  app->tag_fetcher()->Fetch(app->player()->current_song());
}

void Dialogs::EditTag(GtkWindow *parent, Application *app) {
  Song song = app->player()->current_song();
  if (app->playlist_manager()->active() && app->playlist_manager()->current_row() >= 0) {
    song = app->playlist_manager()->current_song();
  }
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Edit tags");
  adw_dialog_set_content_width(dialog, 480);
  adw_dialog_set_content_height(dialog, 640);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  AdwViewStack *stack = ADW_VIEW_STACK(adw_view_stack_new());
  GtkWidget *switcher = adw_view_switcher_new();
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher), stack);

  auto *fields = new std::vector<std::pair<std::string, GtkWidget *>>();
  auto add_page = [&](const char *name, const std::vector<std::pair<const char *, std::string>> &rows, bool lyrics = false) {
    GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
    gtk_widget_set_margin_start(page, 12);
    gtk_widget_set_margin_end(page, 12);
    gtk_widget_set_margin_top(page, 12);
    for (const auto &row : rows) {
      if (lyrics) {
        GtkWidget *view = gtk_text_view_new();
        gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(view), GTK_WRAP_WORD);
        gtk_widget_set_vexpand(view, TRUE);
        GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
        gtk_text_buffer_set_text(buffer, row.second.c_str(), -1);
        fields->emplace_back(row.first, view);
        gtk_box_append(GTK_BOX(page), gtk_label_new(row.first));
        gtk_box_append(GTK_BOX(page), view);
      } else {
        AdwEntryRow *entry = ADW_ENTRY_ROW(adw_entry_row_new());
        adw_preferences_row_set_title(ADW_PREFERENCES_ROW(entry), row.first);
        gtk_editable_set_text(GTK_EDITABLE(entry), row.second.c_str());
        fields->emplace_back(row.first, GTK_WIDGET(entry));
        gtk_box_append(GTK_BOX(page), GTK_WIDGET(entry));
      }
    }
    adw_view_stack_add_titled(stack, page, name, name);
  };

  add_page("Summary", {{"Title", song.title()}, {"Artist", song.artist()}, {"Album", song.album()}, {"Album artist", song.albumartist()},
                       {"Year", song.year() > 0 ? std::to_string(song.year()) : ""}, {"Track", song.track() > 0 ? std::to_string(song.track()) : ""},
                       {"Genre", song.genre()}});
  add_page("Tags", {{"Composer", song.composer()}, {"Performer", song.performer()}, {"Grouping", song.grouping()},
                    {"Comment", song.comment()}, {"Disc", song.disc() > 0 ? std::to_string(song.disc()) : ""},
                    {"BPM", song.bpm() > 0 ? std::to_string(song.bpm()) : ""}, {"Mood", song.mood()}, {"Initial key", song.initial_key()}});
  add_page("Lyrics", {{"Lyrics", song.lyrics()}}, true);

  GtkWidget *save = gtk_button_new_with_label("Save");
  gtk_widget_add_css_class(save, "suggested-action");
  auto *copy = new Song(song);
  g_object_set_data_full(G_OBJECT(save), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  g_object_set_data_full(G_OBJECT(save), "fields", fields, [](gpointer p) { delete static_cast<std::vector<std::pair<std::string, GtkWidget *>> *>(p); });
  g_signal_connect(save, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(button), "song"));
                     auto *fields = static_cast<std::vector<std::pair<std::string, GtkWidget *>> *>(g_object_get_data(G_OBJECT(button), "fields"));
                     auto text_of = [](GtkWidget *widget) -> std::string {
                       if (GTK_IS_TEXT_VIEW(widget)) {
                         GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
                         GtkTextIter start, end;
                         gtk_text_buffer_get_bounds(buffer, &start, &end);
                         gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
                         std::string result = text ? text : "";
                         g_free(text);
                         return result;
                       }
                       return gtk_editable_get_text(GTK_EDITABLE(widget));
                     };
                     for (const auto &field : *fields) {
                       const std::string value = text_of(field.second);
                       if (field.first == "Title") song->set_title(value);
                       else if (field.first == "Artist") song->set_artist(value);
                       else if (field.first == "Album") song->set_album(value);
                       else if (field.first == "Album artist") song->set_albumartist(value);
                       else if (field.first == "Composer") song->set_composer(value);
                       else if (field.first == "Performer") song->set_performer(value);
                       else if (field.first == "Grouping") song->set_grouping(value);
                       else if (field.first == "Comment") song->set_comment(value);
                       else if (field.first == "Genre") song->set_genre(value);
                       else if (field.first == "Lyrics") song->set_lyrics(value);
                       else if (field.first == "Mood") song->set_mood(value);
                       else if (field.first == "Initial key") song->set_initial_key(value);
                       else if (field.first == "Year") song->set_year(std::atoi(value.c_str()));
                       else if (field.first == "Track") song->set_track(std::atoi(value.c_str()));
                       else if (field.first == "Disc") song->set_disc(std::atoi(value.c_str()));
                       else if (field.first == "BPM") song->set_bpm(std::strtof(value.c_str(), nullptr));
                     }
                     application->tagreader()->WriteFile(*song);
                     if (song->id() > 0) {
                       application->collection()->backend()->AddOrUpdateSong(*song);
                     }
                     gtk_button_set_label(button, "Saved");
                   })),
                   app);
  gtk_box_append(GTK_BOX(box), switcher);
  gtk_widget_set_vexpand(GTK_WIDGET(stack), TRUE);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(stack));
  gtk_box_append(GTK_BOX(box), save);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::Shortcuts(GtkWindow *parent) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Keyboard shortcuts");
  GtkWidget *label = gtk_label_new(
      "Space  Play/Pause\nCtrl+Right  Next\nCtrl+Left  Previous\nCtrl+Up  Volume up\nCtrl+Down  Volume down\n"
      "Ctrl+Z  Undo\nCtrl+Shift+Z  Redo\nCtrl+N  New playlist\nCtrl+O  Open files\nCtrl+S  Save playlist\n"
      "Ctrl+Q  Quit\nCtrl+,  Preferences");
  gtk_widget_set_margin_start(label, 24);
  gtk_widget_set_margin_end(label, 24);
  gtk_widget_set_margin_top(label, 24);
  gtk_widget_set_margin_bottom(label, 24);
  adw_dialog_set_child(dialog, label);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::GrabShortcut(GtkWindow *parent, const std::function<void(const std::string &)> &callback) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Grab shortcut", "Press a key combination."));
  GtkWidget *label = gtk_label_new("Waiting…");
  adw_alert_dialog_set_extra_child(dialog, label);
  adw_alert_dialog_add_response(dialog, "cancel", "Cancel");
  auto *cb = new std::function<void(const std::string &)>(callback);
  g_object_set_data(G_OBJECT(dialog), "label", label);
  g_object_set_data(G_OBJECT(dialog), "callback", cb);
  GtkEventController *keys = gtk_event_controller_key_new();
  g_signal_connect(keys, "key-pressed", G_CALLBACK(+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     AdwAlertDialog *alert = ADW_ALERT_DIALOG(data);
                     auto *fn = static_cast<std::function<void(const std::string &)> *>(g_object_get_data(G_OBJECT(alert), "callback"));
                     GtkWidget *lab = GTK_WIDGET(g_object_get_data(G_OBJECT(alert), "label"));
                     gchar *name = gtk_accelerator_name(keyval, state);
                     gtk_label_set_text(GTK_LABEL(lab), name);
                     if (fn) {
                       (*fn)(name ? name : "");
                     }
                     g_free(name);
                     return TRUE;
                   }),
                   dialog);
  gtk_widget_add_controller(GTK_WIDGET(dialog), keys);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *, const char *, gpointer data) {
                     delete static_cast<std::function<void(const std::string &)> *>(data);
                   }),
                   cb);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

void Dialogs::Login(GtkWindow *parent, const std::string &service, const std::function<void(const std::string &, const std::string &)> &callback) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(("Sign in to " + service).c_str(), "Enter username and password or token."));
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  GtkWidget *user = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(user), "Username");
  GtkWidget *pass = gtk_password_entry_new();
  gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(pass), TRUE);
  gtk_box_append(GTK_BOX(box), user);
  gtk_box_append(GTK_BOX(box), pass);
  adw_alert_dialog_set_extra_child(dialog, box);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "login", "Sign in", nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "login", ADW_RESPONSE_SUGGESTED);
  auto *cb = new std::function<void(const std::string &, const std::string &)>(callback);
  g_object_set_data(G_OBJECT(dialog), "user", user);
  g_object_set_data(G_OBJECT(dialog), "pass", pass);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     auto *fn = static_cast<std::function<void(const std::string &, const std::string &)> *>(data);
                     if (g_strcmp0(response, "login") == 0) {
                       (*fn)(gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "user"))),
                             gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "pass"))));
                     }
                     delete fn;
                   }),
                   cb);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

void Dialogs::SmartPlaylistWizard(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Smart playlist");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Playlist name");
  static const char *field_names[] = {"Title", "Album", "Artist", "Genre", "Year", "Rating", "Playcount", nullptr};
  static const char *op_names[] = {"Contains", "Equals", "Greater than", "Less than", "Not contains", nullptr};
  GtkWidget *field = gtk_drop_down_new_from_strings(field_names);
  GtkWidget *op = gtk_drop_down_new_from_strings(op_names);
  GtkWidget *value = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(value), "Value");
  GtkWidget *limit = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(limit), 100);
  GtkWidget *create = gtk_button_new_with_label("Create");
  gtk_widget_add_css_class(create, "suggested-action");
  g_object_set_data(G_OBJECT(create), "name", name);
  g_object_set_data(G_OBJECT(create), "field", field);
  g_object_set_data(G_OBJECT(create), "op", op);
  g_object_set_data(G_OBJECT(create), "value", value);
  g_object_set_data(G_OBJECT(create), "limit", limit);
  g_signal_connect(create, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     SmartPlaylistSearch search;
                     const guint field_i = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "field")));
                     const guint op_i = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "op")));
                     static const SmartPlaylistField fields[] = {SmartPlaylistField::Title, SmartPlaylistField::Album, SmartPlaylistField::Artist,
                                                                SmartPlaylistField::Genre, SmartPlaylistField::Year, SmartPlaylistField::Rating,
                                                                SmartPlaylistField::Playcount};
                     static const SmartPlaylistOp ops[] = {SmartPlaylistOp::Contains, SmartPlaylistOp::Equals, SmartPlaylistOp::GreaterThan,
                                                          SmartPlaylistOp::LessThan, SmartPlaylistOp::NotContains};
                     search.terms.push_back({fields[field_i], ops[op_i], gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "value")))});
                     search.limit = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "limit"))));
                     const char *playlist_name = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "name")));
                     application->playlist_manager()->New(playlist_name && *playlist_name ? playlist_name : "Smart playlist");
                     application->playlist_manager()->AppendSongs(search.Search(application->collection()->Songs()));
                   })),
                   app);
  gtk_box_append(GTK_BOX(box), name);
  gtk_box_append(GTK_BOX(box), field);
  gtk_box_append(GTK_BOX(box), op);
  gtk_box_append(GTK_BOX(box), value);
  gtk_box_append(GTK_BOX(box), limit);
  gtk_box_append(GTK_BOX(box), create);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::GroupBy(GtkWindow *parent, const std::function<void(const std::string &)> &callback) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Collection grouping", "Choose how albums are grouped."));
  static const char *group_labels[] = {"Artist – Album", "Album", "Genre", "Year", "Artist", nullptr};
  GtkWidget *drop = gtk_drop_down_new_from_strings(group_labels);
  adw_alert_dialog_set_extra_child(dialog, drop);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "apply", "Apply", nullptr);
  auto *cb = new std::function<void(const std::string &)>(callback);
  g_object_set_data(G_OBJECT(dialog), "drop", drop);
  g_signal_connect(dialog, "response", G_CALLBACK((+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     auto *fn = static_cast<std::function<void(const std::string &)> *>(data);
                     if (g_strcmp0(response, "apply") == 0) {
                       static const char *values[] = {"artist-album", "album", "genre", "year", "artist"};
                       const guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(alert), "drop")));
                       (*fn)(values[selected]);
                     }
                     delete fn;
                   })),
                   cb);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

void Dialogs::Console(GtkWindow *parent) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Debug console");
  GtkWidget *view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), "Logging is written to the GLib log domain \"strawberry\".", -1);
  adw_dialog_set_child(dialog, view);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::Error(GtkWindow *parent, const std::string &message) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Error", message.c_str()));
  adw_alert_dialog_add_response(dialog, "ok", "OK");
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
