#include "ui/dialogs.h"

#include "collection/collectiongrouping.h"
#include "core/application.h"
#include "core/settings.h"
#include "playlistparsers/playlistparser.h"
#include "equalizer/equalizer.h"
#include "organize/organize.h"
#include "smartplaylists/smartplaylist.h"
#include "transcoder/transcoder.h"
#include "utilities/fileutils.h"
#include "utilities/jsonutils.h"
#include "utilities/timeutils.h"

#include <adwaita.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <vector>

namespace {

Song SongForDialog(Application *app) {
  Song song = app->player()->current_song();
  if (app->playlist_manager()->active() && app->playlist_manager()->current_row() >= 0) {
    song = app->playlist_manager()->current_song();
  }
  return song;
}

void SetImageFromBytes(GtkWidget *image, const std::vector<unsigned char> &data, int pixel_size) {
  if (!image) {
    return;
  }
  if (data.empty()) {
    gtk_image_set_from_icon_name(GTK_IMAGE(image), "audio-x-generic-symbolic");
    gtk_image_set_pixel_size(GTK_IMAGE(image), pixel_size);
    return;
  }
  GdkPixbufLoader *loader = gdk_pixbuf_loader_new();
  if (gdk_pixbuf_loader_write(loader, data.data(), data.size(), nullptr) && gdk_pixbuf_loader_close(loader, nullptr)) {
    GdkPixbuf *pixbuf = gdk_pixbuf_loader_get_pixbuf(loader);
    if (pixbuf) {
      GdkPixbuf *scaled = gdk_pixbuf_scale_simple(pixbuf, pixel_size, pixel_size, GDK_INTERP_BILINEAR);
      GdkTexture *texture = gdk_texture_new_for_pixbuf(scaled);
      gtk_image_set_from_paintable(GTK_IMAGE(image), GDK_PAINTABLE(texture));
      g_object_unref(texture);
      g_object_unref(scaled);
    }
  }
  g_object_unref(loader);
}

std::string PrettyBytes(int64_t bytes) {
  if (bytes < 0) {
    return {};
  }
  if (bytes < 1024) {
    return std::to_string(bytes) + " B";
  }
  if (bytes < 1024 * 1024) {
    return std::to_string(bytes / 1024) + " KB";
  }
  char buf[32];
  g_snprintf(buf, sizeof(buf), "%.1f MB", static_cast<double>(bytes) / 1048576.0);
  return buf;
}

std::string PrettyUnixTime(int64_t ts) {
  if (ts <= 0) {
    return "Never";
  }
  const time_t value = static_cast<time_t>(ts);
  struct tm local {};
  localtime_r(&value, &local);
  char buf[64];
  strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", &local);
  return buf;
}

std::string SafeFolderName(std::string name) {
  for (char &ch : name) {
    if (ch == '/' || ch == '\\' || ch == ':' || ch == '\0') {
      ch = '-';
    }
  }
  return name.empty() ? "Unknown" : name;
}

GtkWidget *DropDownFromNames(const std::vector<std::string> &names) {
  GtkStringList *list = gtk_string_list_new(nullptr);
  for (const std::string &name : names) {
    gtk_string_list_append(list, name.c_str());
  }
  return gtk_drop_down_new(G_LIST_MODEL(list), nullptr);
}

bool ApplyCover(Application *app, Song *song, const std::string &image) {
  if (!song || image.empty() || !CoverProviders::SaveAlbumCover(*song, image, app->tagreader())) {
    return false;
  }
  const std::string dest = FileUtils::Join(FileUtils::DirName(FileUtils::PathFromUri(song->url())), "cover.jpg");
  song->set_art_manual(FileUtils::UriFromPath(dest));
  song->set_art_unset(false);
  song->set_art_embedded(true);
  if (song->id() > 0) {
    app->collection()->backend()->AddOrUpdateSong(*song);
  }
  return true;
}

struct SmartTermRow {
  GtkWidget *field = nullptr;
  GtkWidget *op = nullptr;
  GtkWidget *value = nullptr;
};

struct SmartWizard {
  GtkWidget *name = nullptr;
  GtkWidget *match = nullptr;
  SmartTermRow terms[3];
  GtkWidget *limit = nullptr;
  GtkWidget *sort = nullptr;
  GtkWidget *descending = nullptr;
  GtkWidget *dynamic = nullptr;
  GtkWidget *preview = nullptr;
};

struct CoverExportJob {
  Application *app = nullptr;
  std::string filename;
  bool overwrite = true;
};

SmartPlaylistSearch SearchFromWizard(SmartWizard *wizard) {
  SmartPlaylistSearch search;
  search.type = gtk_drop_down_get_selected(GTK_DROP_DOWN(wizard->match)) == 1 ? SmartPlaylistSearch::SearchType::Or
                                                                             : SmartPlaylistSearch::SearchType::And;
  for (const SmartTermRow &term : wizard->terms) {
    const int field_i = static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(term.field)));
    const int op_i = static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(term.op)));
    const SmartPlaylistOp op = SmartPlaylistSearch::OpFromIndex(op_i);
    const std::string value = gtk_editable_get_text(GTK_EDITABLE(term.value));
    if (value.empty() && op != SmartPlaylistOp::Empty && op != SmartPlaylistOp::NotEmpty) {
      continue;
    }
    search.terms.push_back({SmartPlaylistSearch::FieldFromIndex(field_i), op, value});
  }
  search.limit = static_cast<int>(gtk_spin_button_get_value(GTK_SPIN_BUTTON(wizard->limit)));
  search.sort_field = SmartPlaylistSearch::FieldFromIndex(static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(wizard->sort))));
  search.sort_descending = gtk_check_button_get_active(GTK_CHECK_BUTTON(wizard->descending));
  return search;
}

}  // namespace

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
    g_signal_connect(button, "clicked", G_CALLBACK((+[](GtkButton *btn, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(btn), "song"));
                       if (!song) {
                         return;
                       }
                       application->cover_providers()->Fetch(*song, [btn, application, song](const std::string &image, const std::string &error) {
                         if (!image.empty() && CoverProviders::SaveAlbumCover(*song, image, application->tagreader())) {
                           song->set_art_manual(FileUtils::UriFromPath(FileUtils::Join(FileUtils::DirName(FileUtils::PathFromUri(song->url())), "cover.jpg")));
                           if (song->id() > 0) {
                             application->collection()->backend()->AddOrUpdateSong(*song);
                           }
                           gtk_button_set_label(GTK_BUTTON(btn), "Saved");
                           return;
                         }
                         gtk_button_set_label(GTK_BUTTON(btn), image.empty() ? (error.empty() ? "Missing" : "Failed") : "Failed");
                       });
                     })),
                     app);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), button);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
  }
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  gtk_box_append(GTK_BOX(box), scroll);
  GtkWidget *current = gtk_button_new_with_label("Fetch cover for current song");
  g_signal_connect(current, "clicked", G_CALLBACK((+[](GtkButton *btn, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     Song song = application->player()->current_song();
                     application->cover_providers()->Fetch(song, [application, song, btn](const std::string &image, const std::string &) {
                       if (!image.empty() && CoverProviders::SaveAlbumCover(song, image, application->tagreader())) {
                         gtk_button_set_label(btn, "Saved");
                       }
                     });
                   })),
                   app);
  GtkWidget *from_url = gtk_button_new_with_label("Load cover from URL…");
  g_signal_connect(from_url, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     Dialogs::CoverFromUrl(nullptr, application);
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), current);
  gtk_box_append(GTK_BOX(box), from_url);
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
  GtkStringList *preset_names = gtk_string_list_new(nullptr);
  for (const std::string &name : equalizer->Presets()) {
    gtk_string_list_append(preset_names, name.c_str());
  }
  GtkWidget *preset = gtk_drop_down_new(G_LIST_MODEL(preset_names), nullptr);
  g_signal_connect(preset, "notify::selected", G_CALLBACK(+[](GtkDropDown *drop, GParamSpec *, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkStringObject *item = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(drop));
                     if (item) {
                       eq->LoadPreset(gtk_string_object_get_string(item));
                     }
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(box), preset);
  GtkWidget *preset_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *preset_name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(preset_name), "Custom preset name");
  GtkWidget *save_preset = gtk_button_new_with_label("Save");
  GtkWidget *delete_preset = gtk_button_new_with_label("Delete");
  g_object_set_data(G_OBJECT(save_preset), "name", preset_name);
  g_object_set_data(G_OBJECT(save_preset), "list", preset_names);
  g_signal_connect(save_preset, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkWidget *entry = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "name"));
                     const char *name = gtk_editable_get_text(GTK_EDITABLE(entry));
                     if (eq->SavePreset(name ? name : "")) {
                       gtk_string_list_append(GTK_STRING_LIST(g_object_get_data(G_OBJECT(button), "list")), name);
                     }
                   }),
                   equalizer);
  g_object_set_data(G_OBJECT(delete_preset), "drop", preset);
  g_signal_connect(delete_preset, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *eq = static_cast<class Equalizer *>(data);
                     GtkDropDown *drop = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "drop"));
                     GtkStringObject *item = GTK_STRING_OBJECT(gtk_drop_down_get_selected_item(drop));
                     if (item) {
                       eq->DeletePreset(gtk_string_object_get_string(item));
                     }
                   }),
                   equalizer);
  gtk_box_append(GTK_BOX(preset_row), preset_name);
  gtk_box_append(GTK_BOX(preset_row), save_preset);
  gtk_box_append(GTK_BOX(preset_row), delete_preset);
  gtk_box_append(GTK_BOX(box), preset_row);
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
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  GtkWidget *preview = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(preview), TRUE);
  gtk_label_set_xalign(GTK_LABEL(preview), 0.0f);
  GtkWidget *preview_btn = gtk_button_new_with_label("Preview");
  GtkWidget *run = gtk_button_new_with_label("Organize");
  gtk_widget_add_css_class(run, "suggested-action");
  g_object_set_data(G_OBJECT(run), "format", entry);
  g_object_set_data(G_OBJECT(run), "dest", dest);
  g_object_set_data(G_OBJECT(run), "move", move);
  g_object_set_data(G_OBJECT(run), "status", status);
  g_object_set_data(G_OBJECT(preview_btn), "format", entry);
  g_object_set_data(G_OBJECT(preview_btn), "dest", dest);
  g_object_set_data(G_OBJECT(preview_btn), "preview", preview);
  g_signal_connect(preview_btn, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     if (!application->playlist_manager()->active()) {
                       return;
                     }
                     OrganizeFormat format(gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "format"))));
                     const std::string dest_dir = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest")));
                     std::string text;
                     const SongList songs = application->playlist_manager()->active()->songs();
                     for (size_t i = 0; i < songs.size() && i < 8; ++i) {
                       text += FileUtils::Join(dest_dir, format.GetFilenameForSong(songs[i])) + "\n";
                     }
                     if (songs.size() > 8) {
                       text += "… " + std::to_string(songs.size() - 8) + " more";
                     }
                     gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "preview")), text.c_str());
                   }),
                   app);
  g_signal_connect(run, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     if (!application->playlist_manager()->active()) {
                       return;
                     }
                     OrganizeFormat format(gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "format"))));
                     const std::string dest_dir = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest")));
                     const bool move_files = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "move")));
                     class Organize organize;
                     const auto errors = organize.Copy(application->playlist_manager()->active()->songs(), dest_dir, format, move_files);
                     GtkWidget *status_label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "status"));
                     if (errors.empty()) {
                       gtk_label_set_text(GTK_LABEL(status_label), "Organize finished.");
                       return;
                     }
                     std::string text = std::to_string(errors.size()) + " file(s) failed:\n";
                     for (size_t i = 0; i < errors.size() && i < 12; ++i) {
                       text += errors[i].song + " — " + errors[i].message + "\n";
                     }
                     gtk_label_set_text(GTK_LABEL(status_label), text.c_str());
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Filename format"));
  gtk_box_append(GTK_BOX(box), entry);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Destination"));
  gtk_box_append(GTK_BOX(box), dest);
  gtk_box_append(GTK_BOX(box), move);
  gtk_box_append(GTK_BOX(box), preview_btn);
  gtk_box_append(GTK_BOX(box), preview);
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
  app->tag_fetcher()->Results.Connect([status, list, app](const SongList &songs) {
    gtk_label_set_text(GTK_LABEL(status), songs.empty() ? "No matches" : (std::to_string(songs.size()) + " matches — click Apply to write tags").c_str());
    for (const Song &song : songs) {
      auto *copy = new Song(song);
      GtkWidget *row = adw_action_row_new();
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), song.PrettyTitle().c_str());
      adw_action_row_set_subtitle(ADW_ACTION_ROW(row), (song.artist() + " – " + song.album() + (song.year() > 0 ? " (" + std::to_string(song.year()) + ")" : "")).c_str());
      GtkWidget *apply = gtk_button_new_with_label("Apply");
      gtk_widget_add_css_class(apply, "suggested-action");
      g_object_set_data_full(G_OBJECT(apply), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
      g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                         auto *application = static_cast<Application *>(data);
                         auto *result = static_cast<Song *>(g_object_get_data(G_OBJECT(btn), "song"));
                         if (!result) {
                           return;
                         }
                         application->tagreader()->WriteFile(*result);
                         if (result->id() > 0) {
                           application->collection()->backend()->AddOrUpdateSong(*result);
                         }
                         gtk_button_set_label(btn, "Applied");
                       }),
                       app);
      adw_action_row_add_suffix(ADW_ACTION_ROW(row), apply);
      gtk_list_box_append(GTK_LIST_BOX(list), row);
    }
  });
  app->tag_fetcher()->Fetch(app->player()->current_song());
}

void Dialogs::EditTag(GtkWindow *parent, Application *app) {
  const Song song = SongForDialog(app);
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Edit tags");
  adw_dialog_set_content_width(dialog, 520);
  adw_dialog_set_content_height(dialog, 720);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  AdwViewStack *stack = ADW_VIEW_STACK(adw_view_stack_new());
  GtkWidget *switcher = adw_view_switcher_new();
  adw_view_switcher_set_stack(ADW_VIEW_SWITCHER(switcher), stack);

  struct State {
    Song song;
    GtkWidget *cover = nullptr;
    GtkWidget *lyrics = nullptr;
    GtkWidget *rating = nullptr;
    GtkWidget *compilation = nullptr;
    std::vector<std::pair<std::string, GtkWidget *>> fields;
  };
  auto *state = new State{song};

  auto add_entries = [&](GtkWidget *page, const std::vector<std::pair<const char *, std::string>> &rows) {
    for (const auto &row : rows) {
      AdwEntryRow *entry = ADW_ENTRY_ROW(adw_entry_row_new());
      adw_preferences_row_set_title(ADW_PREFERENCES_ROW(entry), row.first);
      gtk_editable_set_text(GTK_EDITABLE(entry), row.second.c_str());
      state->fields.emplace_back(row.first, GTK_WIDGET(entry));
      gtk_box_append(GTK_BOX(page), GTK_WIDGET(entry));
    }
  };

  GtkWidget *summary = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(summary, 12);
  gtk_widget_set_margin_end(summary, 12);
  gtk_widget_set_margin_top(summary, 12);
  state->cover = gtk_image_new();
  gtk_widget_set_halign(state->cover, GTK_ALIGN_CENTER);
  SetImageFromBytes(state->cover, app->albumcover_loader()->LoadData(song), 160);
  gtk_box_append(GTK_BOX(summary), state->cover);
  GtkWidget *cover_buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_widget_set_halign(cover_buttons, GTK_ALIGN_CENTER);
  GtkWidget *fetch_cover = gtk_button_new_with_label("Fetch cover");
  GtkWidget *search_cover = gtk_button_new_with_label("Search…");
  GtkWidget *unset_cover = gtk_button_new_with_label("Unset");
  gtk_box_append(GTK_BOX(cover_buttons), fetch_cover);
  gtk_box_append(GTK_BOX(cover_buttons), search_cover);
  gtk_box_append(GTK_BOX(cover_buttons), unset_cover);
  gtk_box_append(GTK_BOX(summary), cover_buttons);
  const std::string stats = "Plays: " + std::to_string(song.playcount()) + "   Skips: " + std::to_string(song.skipcount()) +
                            "   Last played: " + PrettyUnixTime(song.lastplayed()) + "\n" +
                            FileUtils::PathFromUri(song.url()) + "\n" + PrettyBytes(song.filesize()) + " · " +
                            Utilities::PrettyTimeNanosec(song.length_nanosec()) + " · " +
                            (song.bitrate() > 0 ? std::to_string(song.bitrate()) + " kbps" : "") +
                            (song.samplerate() > 0 ? " · " + std::to_string(song.samplerate()) + " Hz" : "") +
                            (song.bitdepth() > 0 ? " · " + std::to_string(song.bitdepth()) + "-bit" : "") + " · " +
                            Song::FiletypeToString(song.filetype());
  GtkWidget *stats_label = gtk_label_new(stats.c_str());
  gtk_label_set_wrap(GTK_LABEL(stats_label), TRUE);
  gtk_label_set_xalign(GTK_LABEL(stats_label), 0);
  gtk_widget_add_css_class(stats_label, "dim-label");
  gtk_box_append(GTK_BOX(summary), stats_label);
  GtkWidget *rating_label = gtk_label_new("Rating");
  gtk_label_set_xalign(GTK_LABEL(rating_label), 0);
  gtk_box_append(GTK_BOX(summary), rating_label);
  state->rating = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0, 5, 0.5);
  gtk_scale_set_digits(GTK_SCALE(state->rating), 1);
  gtk_scale_set_draw_value(GTK_SCALE(state->rating), TRUE);
  gtk_range_set_value(GTK_RANGE(state->rating), song.rating() >= 0 ? song.rating() * 5.0 : 0);
  gtk_box_append(GTK_BOX(summary), state->rating);
  add_entries(summary, {{"Title", song.title()},
                        {"Artist", song.artist()},
                        {"Album", song.album()},
                        {"Album artist", song.albumartist()},
                        {"Year", song.year() > 0 ? std::to_string(song.year()) : ""},
                        {"Original year", song.originalyear() > 0 ? std::to_string(song.originalyear()) : ""},
                        {"Track", song.track() > 0 ? std::to_string(song.track()) : ""},
                        {"Genre", song.genre()}});
  adw_view_stack_add_titled(stack, summary, "Summary", "Summary");

  GtkWidget *tags = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(tags, 12);
  gtk_widget_set_margin_end(tags, 12);
  gtk_widget_set_margin_top(tags, 12);
  state->compilation = gtk_check_button_new_with_label("Compilation");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(state->compilation), song.compilation());
  gtk_box_append(GTK_BOX(tags), state->compilation);
  add_entries(tags, {{"Composer", song.composer()},
                     {"Performer", song.performer()},
                     {"Grouping", song.grouping()},
                     {"Comment", song.comment()},
                     {"Disc", song.disc() > 0 ? std::to_string(song.disc()) : ""},
                     {"BPM", song.bpm() > 0 ? std::to_string(song.bpm()) : ""},
                     {"Mood", song.mood()},
                     {"Initial key", song.initial_key()},
                     {"Title sort", song.titlesort()},
                     {"Artist sort", song.artistsort()},
                     {"Album sort", song.albumsort()},
                     {"Album artist sort", song.albumartistsort()}});
  adw_view_stack_add_titled(stack, tags, "Tags", "Tags");

  GtkWidget *lyrics_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 6);
  gtk_widget_set_margin_start(lyrics_page, 12);
  gtk_widget_set_margin_end(lyrics_page, 12);
  gtk_widget_set_margin_top(lyrics_page, 12);
  state->lyrics = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(state->lyrics), GTK_WRAP_WORD);
  gtk_widget_set_vexpand(state->lyrics, TRUE);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->lyrics)), song.lyrics().c_str(), -1);
  state->fields.emplace_back("Lyrics", state->lyrics);
  gtk_box_append(GTK_BOX(lyrics_page), gtk_label_new("Lyrics"));
  gtk_box_append(GTK_BOX(lyrics_page), state->lyrics);
  adw_view_stack_add_titled(stack, lyrics_page, "Lyrics", "Lyrics");

  GtkWidget *save = gtk_button_new_with_label("Save");
  gtk_widget_add_css_class(save, "suggested-action");
  g_object_set_data_full(G_OBJECT(save), "state", state, [](gpointer p) { delete static_cast<State *>(p); });
  g_signal_connect(save, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     auto text_of = [](GtkWidget *widget) -> std::string {
                       if (GTK_IS_TEXT_VIEW(widget)) {
                         GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widget));
                         GtkTextIter start;
                         GtkTextIter end;
                         gtk_text_buffer_get_bounds(buffer, &start, &end);
                         gchar *text = gtk_text_buffer_get_text(buffer, &start, &end, FALSE);
                         std::string result = text ? text : "";
                         g_free(text);
                         return result;
                       }
                       return gtk_editable_get_text(GTK_EDITABLE(widget));
                     };
                     for (const auto &field : state->fields) {
                       const std::string value = text_of(field.second);
                       if (field.first == "Title") state->song.set_title(value);
                       else if (field.first == "Artist") state->song.set_artist(value);
                       else if (field.first == "Album") state->song.set_album(value);
                       else if (field.first == "Album artist") state->song.set_albumartist(value);
                       else if (field.first == "Composer") state->song.set_composer(value);
                       else if (field.first == "Performer") state->song.set_performer(value);
                       else if (field.first == "Grouping") state->song.set_grouping(value);
                       else if (field.first == "Comment") state->song.set_comment(value);
                       else if (field.first == "Genre") state->song.set_genre(value);
                       else if (field.first == "Lyrics") state->song.set_lyrics(value);
                       else if (field.first == "Mood") state->song.set_mood(value);
                       else if (field.first == "Initial key") state->song.set_initial_key(value);
                       else if (field.first == "Title sort") state->song.set_titlesort(value);
                       else if (field.first == "Artist sort") state->song.set_artistsort(value);
                       else if (field.first == "Album sort") state->song.set_albumsort(value);
                       else if (field.first == "Album artist sort") state->song.set_albumartistsort(value);
                       else if (field.first == "Year") state->song.set_year(std::atoi(value.c_str()));
                       else if (field.first == "Original year") state->song.set_originalyear(std::atoi(value.c_str()));
                       else if (field.first == "Track") state->song.set_track(std::atoi(value.c_str()));
                       else if (field.first == "Disc") state->song.set_disc(std::atoi(value.c_str()));
                       else if (field.first == "BPM") state->song.set_bpm(std::strtof(value.c_str(), nullptr));
                     }
                     if (state->compilation) {
                       state->song.set_compilation(gtk_check_button_get_active(GTK_CHECK_BUTTON(state->compilation)));
                     }
                     if (state->rating) {
                       state->song.set_rating(static_cast<float>(gtk_range_get_value(GTK_RANGE(state->rating)) / 5.0));
                     }
                     application->tagreader()->WriteFile(state->song);
                     const std::string path = FileUtils::PathFromUri(state->song.url());
                     if (!path.empty() && state->song.rating() >= 0) {
                       application->tagreader()->SaveRating(path, state->song.rating());
                     }
                     if (state->song.id() > 0) {
                       application->collection()->backend()->AddOrUpdateSong(state->song);
                       application->collection()->backend()->SetRating(state->song.id(), state->song.rating());
                     }
                     gtk_button_set_label(button, "Saved");
                   })),
                   app);

  g_object_set_data(G_OBJECT(fetch_cover), "state", state);
  g_signal_connect(fetch_cover, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     application->cover_providers()->Fetch(state->song, [button, application, state](const std::string &image, const std::string &) {
                       if (ApplyCover(application, &state->song, image)) {
                         SetImageFromBytes(state->cover, std::vector<unsigned char>(image.begin(), image.end()), 160);
                         gtk_button_set_label(button, "Saved");
                       } else {
                         gtk_button_set_label(button, "Failed");
                       }
                     });
                   })),
                   app);
  g_signal_connect(search_cover, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     Dialogs::CoverSearch(nullptr, application);
                   }),
                   app);
  g_object_set_data(G_OBJECT(unset_cover), "state", state);
  g_signal_connect(unset_cover, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     state->song.set_art_unset(true);
                     state->song.set_art_manual({});
                     state->song.set_art_automatic({});
                     state->song.set_art_embedded(false);
                     if (state->song.id() > 0) {
                       application->collection()->backend()->AddOrUpdateSong(state->song);
                     }
                     SetImageFromBytes(state->cover, {}, 160);
                     gtk_button_set_label(button, "Unset");
                   })),
                   app);

  GtkWidget *fetch_lyrics = gtk_button_new_with_label("Fetch lyrics");
  g_object_set_data(G_OBJECT(fetch_lyrics), "state", state);
  g_signal_connect(fetch_lyrics, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *state = static_cast<State *>(g_object_get_data(G_OBJECT(button), "state"));
                     application->lyrics_providers()->Fetch(state->song, [button, state](const std::string &lyrics, const std::string &) {
                       if (lyrics.empty()) {
                         gtk_button_set_label(button, "Missing");
                         return;
                       }
                       state->song.set_lyrics(lyrics);
                       if (state->lyrics) {
                         gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(state->lyrics)), lyrics.c_str(), -1);
                       }
                       gtk_button_set_label(button, "Fetched");
                     });
                   })),
                   app);

  gtk_box_append(GTK_BOX(box), switcher);
  gtk_widget_set_vexpand(GTK_WIDGET(stack), TRUE);
  gtk_box_append(GTK_BOX(box), GTK_WIDGET(stack));
  gtk_box_append(GTK_BOX(box), save);
  gtk_box_append(GTK_BOX(box), fetch_lyrics);
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
  adw_dialog_set_content_width(dialog, 520);
  adw_dialog_set_content_height(dialog, 640);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);
  GtkWidget *name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Playlist name");
  GtkWidget *match = DropDownFromNames({"Match all terms (AND)", "Match any term (OR)"});
  auto *wizard = new SmartWizard();
  wizard->name = name;
  wizard->match = match;
  GtkWidget *terms_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  for (int i = 0; i < 3; ++i) {
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 6);
    wizard->terms[i].field = DropDownFromNames(SmartPlaylistSearch::FieldNames());
    wizard->terms[i].op = DropDownFromNames(SmartPlaylistSearch::OpNames());
    wizard->terms[i].value = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(wizard->terms[i].value), i == 0 ? "Value" : "Optional term");
    gtk_widget_set_hexpand(wizard->terms[i].value, TRUE);
    gtk_box_append(GTK_BOX(row), wizard->terms[i].field);
    gtk_box_append(GTK_BOX(row), wizard->terms[i].op);
    gtk_box_append(GTK_BOX(row), wizard->terms[i].value);
    gtk_box_append(GTK_BOX(terms_box), row);
  }
  wizard->limit = gtk_spin_button_new_with_range(0, 10000, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(wizard->limit), 100);
  wizard->sort = DropDownFromNames(SmartPlaylistSearch::FieldNames());
  wizard->descending = gtk_check_button_new_with_label("Sort descending");
  wizard->dynamic = gtk_check_button_new_with_label("Dynamic (keep refilling as tracks play)");
  wizard->preview = gtk_label_new("Preview shows matching tracks from the collection.");
  gtk_label_set_wrap(GTK_LABEL(wizard->preview), TRUE);
  gtk_label_set_xalign(GTK_LABEL(wizard->preview), 0);
  GtkWidget *preview = gtk_button_new_with_label("Preview");
  GtkWidget *create = gtk_button_new_with_label("Create");
  gtk_widget_add_css_class(create, "suggested-action");
  g_object_set_data_full(G_OBJECT(create), "wizard", wizard, [](gpointer p) { delete static_cast<SmartWizard *>(p); });
  g_object_set_data(G_OBJECT(preview), "wizard", wizard);
  g_signal_connect(preview, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<SmartWizard *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     const SongList songs = SearchFromWizard(wizard).Search(application->collection()->Songs());
                     std::string text = std::to_string(songs.size()) + " matches";
                     for (size_t i = 0; i < songs.size() && i < 8; ++i) {
                       text += "\n" + songs[i].PrettyTitleWithArtist();
                     }
                     gtk_label_set_text(GTK_LABEL(wizard->preview), text.c_str());
                   })),
                   app);
  g_signal_connect(create, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *wizard = static_cast<SmartWizard *>(g_object_get_data(G_OBJECT(button), "wizard"));
                     SmartPlaylistSearch search = SearchFromWizard(wizard);
                     const char *playlist_name = gtk_editable_get_text(GTK_EDITABLE(wizard->name));
                     Playlist *playlist = application->playlist_manager()->New(playlist_name && *playlist_name ? playlist_name : "Smart playlist");
                     if (gtk_check_button_get_active(GTK_CHECK_BUTTON(wizard->dynamic))) {
                       playlist->SetDynamic(true, search);
                       search.limit = search.limit > 0 ? std::min(search.limit, 20) : 20;
                     }
                     application->playlist_manager()->AppendSongs(search.Search(application->collection()->Songs()));
                   })),
                   app);
  gtk_box_append(GTK_BOX(box), name);
  gtk_box_append(GTK_BOX(box), match);
  gtk_box_append(GTK_BOX(box), terms_box);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Limit (0 = no limit)"));
  gtk_box_append(GTK_BOX(box), wizard->limit);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Sort by"));
  gtk_box_append(GTK_BOX(box), wizard->sort);
  gtk_box_append(GTK_BOX(box), wizard->descending);
  gtk_box_append(GTK_BOX(box), wizard->dynamic);
  gtk_box_append(GTK_BOX(box), preview);
  gtk_box_append(GTK_BOX(box), wizard->preview);
  gtk_box_append(GTK_BOX(box), create);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::GroupBy(GtkWindow *parent, const CollectionGrouping::Grouping &current,
                      const std::function<void(const CollectionGrouping::Grouping &)> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Collection grouping");
  adw_dialog_set_content_width(dialog, 420);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  GtkWidget *first = gtk_drop_down_new_from_strings(CollectionGrouping::kComboLabels);
  GtkWidget *second = gtk_drop_down_new_from_strings(CollectionGrouping::kComboLabels);
  GtkWidget *third = gtk_drop_down_new_from_strings(CollectionGrouping::kComboLabels);
  gtk_drop_down_set_selected(GTK_DROP_DOWN(first), CollectionGrouping::ComboIndex(current.first));
  gtk_drop_down_set_selected(GTK_DROP_DOWN(second), CollectionGrouping::ComboIndex(current.second));
  gtk_drop_down_set_selected(GTK_DROP_DOWN(third), CollectionGrouping::ComboIndex(current.third));
  GtkWidget *separate = gtk_check_button_new_with_label("Separate albums by grouping tag");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(separate), CollectionGrouping::SeparateAlbumsByGrouping());
  GtkWidget *name = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(name), "Save as…");
  GtkWidget *buttons = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *apply = gtk_button_new_with_label("Apply");
  gtk_widget_add_css_class(apply, "suggested-action");
  GtkWidget *save = gtk_button_new_with_label("Save");
  GtkWidget *manage = gtk_button_new_with_label("Manage");
  gtk_box_append(GTK_BOX(buttons), apply);
  gtk_box_append(GTK_BOX(buttons), save);
  gtk_box_append(GTK_BOX(buttons), manage);
  auto *cb = new std::function<void(const CollectionGrouping::Grouping &)>(callback);
  g_object_set_data_full(G_OBJECT(dialog), "callback", cb, [](gpointer p) {
    delete static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(p);
  });
  g_object_set_data(G_OBJECT(dialog), "first", first);
  g_object_set_data(G_OBJECT(dialog), "second", second);
  g_object_set_data(G_OBJECT(dialog), "third", third);
  g_object_set_data(G_OBJECT(dialog), "separate", separate);
  g_object_set_data(G_OBJECT(dialog), "name", name);
  g_object_set_data(G_OBJECT(apply), "dialog", dialog);
  g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *fn = static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(data);
                     GtkWidget *dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "dialog"));
                     CollectionGrouping::Grouping grouping;
                     grouping.first = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "first")))));
                     grouping.second = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "second")))));
                     grouping.third = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "third")))));
                     CollectionGrouping::SetSeparateAlbumsByGrouping(
                         gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(dlg), "separate"))));
                     (*fn)(grouping);
                     adw_dialog_close(ADW_DIALOG(dlg));
                   }),
                   cb);
  g_object_set_data(G_OBJECT(save), "dialog", dialog);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     GtkWidget *dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "dialog"));
                     const char *title = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(dlg), "name")));
                     CollectionGrouping::Grouping grouping;
                     grouping.first = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "first")))));
                     grouping.second = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "second")))));
                     grouping.third = CollectionGrouping::FromComboIndex(
                         static_cast<int>(gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(dlg), "third")))));
                     CollectionGrouping::AddSaved(title ? title : "", grouping);
                   }),
                   nullptr);
  g_object_set_data(G_OBJECT(manage), "parent", parent);
  g_signal_connect(manage, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *fn = static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(data);
                     Dialogs::ManageSavedGroupings(GTK_WINDOW(g_object_get_data(G_OBJECT(button), "parent")), *fn);
                   }),
                   cb);
  gtk_box_append(GTK_BOX(box), gtk_label_new("First level"));
  gtk_box_append(GTK_BOX(box), first);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Second level"));
  gtk_box_append(GTK_BOX(box), second);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Third level"));
  gtk_box_append(GTK_BOX(box), third);
  gtk_box_append(GTK_BOX(box), separate);
  gtk_box_append(GTK_BOX(box), name);
  gtk_box_append(GTK_BOX(box), buttons);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::ManageSavedGroupings(GtkWindow *parent, const std::function<void(const CollectionGrouping::Grouping &)> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Saved groupings");
  adw_dialog_set_content_width(dialog, 420);
  adw_dialog_set_content_height(dialog, 360);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  auto *cb = new std::function<void(const CollectionGrouping::Grouping &)>(callback);
  g_object_set_data_full(G_OBJECT(dialog), "callback", cb, [](gpointer p) {
    delete static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(p);
  });
  for (const auto &entry : CollectionGrouping::LoadSaved()) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), entry.first.c_str());
    const std::string subtitle = CollectionGrouping::Label(entry.second.first) + " / " + CollectionGrouping::Label(entry.second.second) +
                                 " / " + CollectionGrouping::Label(entry.second.third);
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), subtitle.c_str());
    GtkWidget *apply = gtk_button_new_with_label("Apply");
    GtkWidget *remove = gtk_button_new_with_label("Remove");
    auto *grouping = new CollectionGrouping::Grouping(entry.second);
    g_object_set_data_full(G_OBJECT(row), "name", g_strdup(entry.first.c_str()), g_free);
    g_object_set_data_full(G_OBJECT(apply), "grouping", grouping, [](gpointer p) { delete static_cast<CollectionGrouping::Grouping *>(p); });
    g_object_set_data(G_OBJECT(apply), "callback", cb);
    g_object_set_data(G_OBJECT(apply), "dialog", dialog);
    g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                       auto *fn = static_cast<std::function<void(const CollectionGrouping::Grouping &)> *>(
                           g_object_get_data(G_OBJECT(button), "callback"));
                       auto *grouping = static_cast<CollectionGrouping::Grouping *>(g_object_get_data(G_OBJECT(button), "grouping"));
                       if (fn && grouping) {
                         (*fn)(*grouping);
                       }
                       adw_dialog_close(ADW_DIALOG(g_object_get_data(G_OBJECT(button), "dialog")));
                     }),
                     nullptr);
    g_object_set_data(G_OBJECT(remove), "row", row);
    g_object_set_data(G_OBJECT(remove), "list", list);
    g_signal_connect(remove, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                       GtkWidget *row = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "row"));
                       const char *name = static_cast<const char *>(g_object_get_data(G_OBJECT(row), "name"));
                       CollectionGrouping::RemoveSaved(name ? name : "");
                       gtk_list_box_remove(GTK_LIST_BOX(g_object_get_data(G_OBJECT(button), "list")), row);
                     }),
                     nullptr);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), apply);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), remove);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
  }
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  gtk_box_append(GTK_BOX(box), scroll);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
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

void Dialogs::CoverFromUrl(GtkWindow *parent, Application *app) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Cover from URL", "Download artwork and save it next to the current song."));
  GtkWidget *entry = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "https://");
  adw_alert_dialog_set_extra_child(dialog, entry);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "fetch", "Download", nullptr);
  g_object_set_data(G_OBJECT(dialog), "entry", entry);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "fetch") != 0) {
                       return;
                     }
                     auto *application = static_cast<Application *>(data);
                     const char *url = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "entry")));
                     if (!url || !*url) {
                       return;
                     }
                     application->network()->Get(url, [application](const NetworkAccessManager::Response &result) {
                       if (result.ok() && JsonUtils::LooksLikeImage(result.body)) {
                         CoverProviders::SaveAlbumCover(application->player()->current_song(), result.body, application->tagreader());
                       }
                     });
                   }),
                   app);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

void Dialogs::CoverSearch(GtkWindow *parent, Application *app) {
  Song song = SongForDialog(app);
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Cover search");
  adw_dialog_set_content_width(dialog, 480);
  adw_dialog_set_content_height(dialog, 560);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 12);
  gtk_widget_set_margin_end(box, 12);
  gtk_widget_set_margin_top(box, 12);
  gtk_widget_set_margin_bottom(box, 12);
  GtkWidget *status = gtk_label_new(("Searching providers for “" + song.album() + "”…").c_str());
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  gtk_box_append(GTK_BOX(box), status);
  gtk_box_append(GTK_BOX(box), scroll);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
  auto *copy = new Song(song);
  g_object_set_data_full(G_OBJECT(dialog), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  g_object_set_data(G_OBJECT(dialog), "list", list);
  g_object_set_data(G_OBJECT(dialog), "status", status);
  g_object_set_data(G_OBJECT(dialog), "count", GINT_TO_POINTER(0));
  app->cover_providers()->FetchAll(song, [dialog, app](const std::string &provider, const std::string &image) {
    if (image.empty() || !GTK_IS_WIDGET(dialog)) {
      return;
    }
    GtkWidget *list = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "list"));
    GtkWidget *status = GTK_WIDGET(g_object_get_data(G_OBJECT(dialog), "status"));
    auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(dialog), "song"));
    const int count = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(dialog), "count")) + 1;
    g_object_set_data(G_OBJECT(dialog), "count", GINT_TO_POINTER(count));
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), provider.c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), (PrettyBytes(static_cast<int64_t>(image.size())) + " image").c_str());
    GtkWidget *thumb = gtk_image_new();
    SetImageFromBytes(thumb, std::vector<unsigned char>(image.begin(), image.end()), 48);
    adw_action_row_add_prefix(ADW_ACTION_ROW(row), thumb);
    GtkWidget *apply = gtk_button_new_with_label("Save");
    gtk_widget_add_css_class(apply, "suggested-action");
    auto *image_copy = new std::string(image);
    g_object_set_data_full(G_OBJECT(apply), "image", image_copy, [](gpointer p) { delete static_cast<std::string *>(p); });
    g_object_set_data(G_OBJECT(apply), "song", song);
    g_signal_connect(apply, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(button), "song"));
                       auto *image = static_cast<std::string *>(g_object_get_data(G_OBJECT(button), "image"));
                       if (song && image && ApplyCover(application, song, *image)) {
                         gtk_button_set_label(button, "Saved");
                       } else {
                         gtk_button_set_label(button, "Failed");
                       }
                     })),
                     app);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), apply);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
    gtk_label_set_text(GTK_LABEL(status), (std::to_string(count) + " covers found").c_str());
  });
}

void Dialogs::CoverExport(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Export album covers");
  adw_dialog_set_content_width(dialog, 420);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  GtkWidget *filename = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(filename), "Filename");
  gtk_editable_set_text(GTK_EDITABLE(filename), "cover.jpg");
  GtkWidget *overwrite = gtk_check_button_new_with_label("Overwrite existing files");
  gtk_check_button_set_active(GTK_CHECK_BUTTON(overwrite), TRUE);
  GtkWidget *status = gtk_label_new("Exports each album’s artwork into Artist - Album folders.");
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  GtkWidget *export_btn = gtk_button_new_with_label("Choose folder…");
  gtk_widget_add_css_class(export_btn, "suggested-action");
  g_object_set_data(G_OBJECT(export_btn), "filename", filename);
  g_object_set_data(G_OBJECT(export_btn), "overwrite", overwrite);
  g_object_set_data(G_OBJECT(export_btn), "status", status);
  g_object_set_data(G_OBJECT(export_btn), "parent", parent);
  g_signal_connect(export_btn, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *job = new CoverExportJob();
                     job->app = application;
                     job->filename = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "filename")));
                     if (job->filename.empty()) {
                       job->filename = "cover.jpg";
                     }
                     job->overwrite = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "overwrite")));
                     GtkWindow *parent = GTK_WINDOW(g_object_get_data(G_OBJECT(button), "parent"));
                     GtkFileDialog *chooser = gtk_file_dialog_new();
                     gtk_file_dialog_set_title(chooser, "Export album covers");
                     gtk_file_dialog_select_folder(chooser, parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
                       auto *job = static_cast<CoverExportJob *>(data);
                       GError *error = nullptr;
                       GFile *folder = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
                       if (!folder) {
                         if (error) {
                           g_error_free(error);
                         }
                         delete job;
                         return;
                       }
                       gchar *path = g_file_get_path(folder);
                       int saved = 0;
                       int skipped = 0;
                       if (path) {
                         std::string last;
                         for (const Song &song : job->app->collection()->Songs()) {
                           const std::string album_key = song.EffectiveAlbumartist() + " – " + song.album();
                           if (album_key == last || song.album().empty()) {
                             continue;
                           }
                           last = album_key;
                           const std::vector<unsigned char> cover = job->app->albumcover_loader()->LoadData(song);
                           if (cover.empty()) {
                             ++skipped;
                             continue;
                           }
                           const std::string dest_dir = FileUtils::Join(path, SafeFolderName(song.EffectiveAlbumartist() + " - " + song.album()));
                           g_mkdir_with_parents(dest_dir.c_str(), 0755);
                           const std::string dest = FileUtils::Join(dest_dir, job->filename);
                           if (!job->overwrite && FileUtils::Exists(dest)) {
                             ++skipped;
                             continue;
                           }
                           if (FileUtils::WriteFile(dest, std::string(cover.begin(), cover.end()))) {
                             ++saved;
                           } else {
                             ++skipped;
                           }
                         }
                         g_free(path);
                       }
                       g_object_unref(folder);
                       AdwAlertDialog *done = ADW_ALERT_DIALOG(adw_alert_dialog_new(
                           "Export album covers", ("Exported " + std::to_string(saved) + " covers (" + std::to_string(skipped) + " skipped).").c_str()));
                       adw_alert_dialog_add_response(done, "ok", "OK");
                       adw_dialog_present(ADW_DIALOG(done), nullptr);
                       delete job;
                     }, job);
                   })),
                   app);
  gtk_box_append(GTK_BOX(box), filename);
  gtk_box_append(GTK_BOX(box), overwrite);
  gtk_box_append(GTK_BOX(box), status);
  gtk_box_append(GTK_BOX(box), export_btn);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::PlaylistColumns(GtkWindow *parent, const std::function<void()> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Playlist columns");
  adw_dialog_set_content_width(dialog, 360);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  static const char *columns[] = {"Track", "Title", "Artist", "Album", "Album artist", "Performer", "Composer", "Year", "Original year",
                                  "Disc", "Length", "Genre", "Sample rate", "Bit depth", "Bitrate", "URL", "Filename", "Filesize",
                                  "Filetype", "Date created", "Date modified", "Plays", "Skips", "Last played", "Comment", "Grouping",
                                  "Source", "Moodbar", "Rating", "CUE", "EBU R128 I", "EBU R128 LRA", "BPM", "Mood", "Initial key", nullptr};
  Settings settings;
  settings.BeginGroup("Playlist");
  const std::string enabled = settings.Value("columns", "Track,Title,Artist,Album,Album artist,Length,Year,Genre,Bitrate,Sample rate,Plays,Rating,Filename");
  GtkWidget *list = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
  for (int i = 0; columns[i]; ++i) {
    GtkWidget *check = gtk_check_button_new_with_label(columns[i]);
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check), enabled.find(columns[i]) != std::string::npos);
    gtk_box_append(GTK_BOX(list), check);
  }
  GtkWidget *scroll = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(scroll, TRUE);
  gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), 360);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), list);
  GtkWidget *apply = gtk_button_new_with_label("Apply");
  gtk_widget_add_css_class(apply, "suggested-action");
  auto *cb = new std::function<void()>(callback);
  g_object_set_data(G_OBJECT(apply), "list", list);
  g_object_set_data_full(G_OBJECT(apply), "callback", cb, [](gpointer p) { delete static_cast<std::function<void()> *>(p); });
  g_signal_connect(apply, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     GtkWidget *list_box = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "list"));
                     std::string value;
                     for (GtkWidget *child = gtk_widget_get_first_child(list_box); child; child = gtk_widget_get_next_sibling(child)) {
                       if (GTK_IS_CHECK_BUTTON(child) && gtk_check_button_get_active(GTK_CHECK_BUTTON(child))) {
                         if (!value.empty()) {
                           value += ",";
                         }
                         value += gtk_check_button_get_label(GTK_CHECK_BUTTON(child));
                       }
                     }
                     Settings settings;
                     settings.BeginGroup("Playlist");
                     settings.SetValue("columns", value);
                     settings.Sync();
                     if (auto *fn = static_cast<std::function<void()> *>(g_object_get_data(G_OBJECT(button), "callback"))) {
                       (*fn)();
                     }
                   }),
                   nullptr);
  gtk_box_append(GTK_BOX(box), scroll);
  gtk_box_append(GTK_BOX(box), apply);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::DeleteFiles(GtkWindow *parent, Application *app) {
  Playlist *playlist = app->playlist_manager()->active();
  if (!playlist || playlist->current_row() < 0) {
    Error(parent, "Select a song in the playlist first.");
    return;
  }
  const Song song = playlist->current_song();
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Delete files", ("Permanently delete “" + song.PrettyTitle() + "” from disk?").c_str()));
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "delete", "Delete", nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "delete", ADW_RESPONSE_DESTRUCTIVE);
  auto *copy = new Song(song);
  g_object_set_data_full(G_OBJECT(dialog), "song", copy, [](gpointer p) { delete static_cast<Song *>(p); });
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     if (g_strcmp0(response, "delete") != 0) {
                       return;
                     }
                     auto *application = static_cast<Application *>(data);
                     auto *song = static_cast<Song *>(g_object_get_data(G_OBJECT(alert), "song"));
                     const std::string path = FileUtils::PathFromUri(song->url());
                     GFile *file = g_file_new_for_path(path.c_str());
                     if (!g_file_trash(file, nullptr, nullptr)) {
                       FileUtils::Remove(path);
                     }
                     g_object_unref(file);
                     if (application->playlist_manager()->active()) {
                       application->playlist_manager()->active()->RemoveRows({application->playlist_manager()->current_row()});
                       application->playlist_manager()->SaveActive();
                     }
                   }),
                   app);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}

void Dialogs::CopyToDevice(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Copy to device");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  app->device_manager()->Rescan();
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  for (const ConnectedDevice &device : app->device_manager()->devices()) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), device.friendly_name.c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), device.mount_path.empty() ? device.backend.c_str() : device.mount_path.c_str());
    GtkWidget *copy = gtk_button_new_with_label("Copy playlist");
    g_object_set_data_full(G_OBJECT(copy), "device-id", g_strdup(device.unique_id.c_str()), g_free);
    g_signal_connect(copy, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "device-id"));
                       if (id && application->playlist_manager()->active()) {
                         const bool ok = application->device_manager()->CopySongs(id, application->playlist_manager()->active()->songs());
                         gtk_button_set_label(btn, ok ? "Copied" : "Failed");
                       }
                     }),
                     app);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), copy);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
  }
  gtk_box_append(GTK_BOX(box), list);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}

void Dialogs::SaveAllPlaylists(GtkWindow *parent, Application *app) {
  GtkFileDialog *chooser = gtk_file_dialog_new();
  gtk_file_dialog_set_title(chooser, "Save all playlists");
  gtk_file_dialog_select_folder(chooser, parent, nullptr, +[](GObject *source, GAsyncResult *result, gpointer data) {
    auto *application = static_cast<Application *>(data);
    GError *error = nullptr;
    GFile *folder = gtk_file_dialog_select_folder_finish(GTK_FILE_DIALOG(source), result, &error);
    if (!folder) {
      if (error) {
        g_error_free(error);
      }
      return;
    }
    gchar *path = g_file_get_path(folder);
    if (path) {
      for (const auto &playlist : application->playlist_manager()->playlists()) {
        const std::string dest = FileUtils::Join(path, playlist->name() + ".m3u");
        PlaylistParser().Save(dest, playlist->songs());
      }
      g_free(path);
    }
    g_object_unref(folder);
  }, app);
}

void Dialogs::Error(GtkWindow *parent, const std::string &message) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Error", message.c_str()));
  adw_alert_dialog_add_response(dialog, "ok", "OK");
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
