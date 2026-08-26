#include "organize/organizedialog.h"

#include "collection/collectionlibrary.h"
#include "constants/organizesettings.h"
#include "core/application.h"
#include "core/settings.h"
#include "organize/organize.h"
#include "organize/organizeerrordialog.h"
#include "organize/organizeformat.h"
#include "organize/organizeformatvalidator.h"
#include "organize/organizesyntaxhighlighter.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"

#include <adwaita.h>

#include <vector>

void OrganizeDialog::Show(GtkWindow *parent, Application *app, const SongList &songs, bool move) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Organize files"));
  adw_dialog_set_content_width(dialog, 560);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);

  Settings settings;
  settings.BeginGroup(OrganizeSettings::kSettingsGroup);
  const std::string saved_format = settings.Value(OrganizeSettings::kFormat, OrganizeSettings::kDefaultFormat);
  const std::string music_dir = g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) ? g_get_user_special_dir(G_USER_DIRECTORY_MUSIC)
                                                                              : g_get_home_dir();
  std::string saved_dest = settings.Value(OrganizeSettings::kDestination, music_dir);
  const std::vector<CollectionDirectory> dirs = app && app->collection() && app->collection()->backend()
                                                   ? app->collection()->backend()->Directories()
                                                   : std::vector<CollectionDirectory>{};
  if (saved_dest.empty() && !dirs.empty()) {
    saved_dest = dirs.front().path;
  }

  GtkWidget *format_header = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(format_header), gtk_label_new(Translations::CStr("Filename format")));
  GtkWidget *insert = gtk_menu_button_new();
  gtk_menu_button_set_label(GTK_MENU_BUTTON(insert), Translations::CStr("Insert tag"));
  gtk_widget_set_hexpand(insert, TRUE);
  gtk_widget_set_halign(insert, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(format_header), insert);

  GtkWidget *format_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(format_view), GTK_WRAP_WORD_CHAR);
  gtk_widget_set_size_request(format_view, -1, 56);
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(format_view));
  OrganizeSyntaxHighlighter highlighter;
  highlighter.Apply(buffer, saved_format);

  GtkWidget *tag_box = gtk_flow_box_new();
  gtk_flow_box_set_max_children_per_line(GTK_FLOW_BOX(tag_box), 3);
  gtk_flow_box_set_selection_mode(GTK_FLOW_BOX(tag_box), GTK_SELECTION_NONE);
  for (int i = 0; OrganizeFormat::kKnownTags[i]; ++i) {
    GtkWidget *btn = gtk_button_new_with_label(OrganizeFormat::kKnownTags[i]);
    g_object_set_data(G_OBJECT(btn), "buffer", buffer);
    g_signal_connect(btn, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                       auto *buf = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(button), "buffer"));
                       const char *token = gtk_button_get_label(button);
                       if (!buf || !token) {
                         return;
                       }
                       GtkTextIter iter;
                       gtk_text_buffer_get_iter_at_mark(buf, &iter, gtk_text_buffer_get_insert(buf));
                       gtk_text_buffer_insert(buf, &iter, token, -1);
                     })),
                     nullptr);
    gtk_flow_box_append(GTK_FLOW_BOX(tag_box), btn);
  }
  GtkWidget *popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER(popover), tag_box);
  gtk_menu_button_set_popover(GTK_MENU_BUTTON(insert), popover);

  GtkWidget *format_error = gtk_label_new("");
  gtk_widget_add_css_class(format_error, "error");
  gtk_label_set_xalign(GTK_LABEL(format_error), 0.0f);
  GtkWidget *dest = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(dest), saved_dest.c_str());
  GtkWidget *dest_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  GtkWidget *browse = gtk_button_new_with_label(Translations::CStr("Choose folder…"));
  gtk_widget_set_hexpand(dest, TRUE);
  gtk_box_append(GTK_BOX(dest_row), dest);
  gtk_box_append(GTK_BOX(dest_row), browse);
  g_object_set_data(G_OBJECT(browse), "dest", dest);
  g_object_set_data(G_OBJECT(browse), "parent", parent);
  g_signal_connect(browse, "clicked", G_CALLBACK((+[](GtkButton *button, gpointer) {
                     GtkFileDialog *chooser = gtk_file_dialog_new();
                     gtk_file_dialog_set_title(chooser, Translations::CStr("Organize destination"));
                     gtk_file_dialog_select_folder(chooser, GTK_WINDOW(g_object_get_data(G_OBJECT(button), "parent")), nullptr,
                                                   +[](GObject *source, GAsyncResult *result, gpointer user) {
                                                     auto *entry = GTK_EDITABLE(user);
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
                                                       gtk_editable_set_text(entry, path);
                                                     }
                                                     g_free(path);
                                                     g_object_unref(folder);
                                                   },
                                                   g_object_get_data(G_OBJECT(button), "dest"));
                   })),
                   nullptr);

  GtkWidget *dest_drop = nullptr;
  if (!dirs.empty()) {
    std::vector<std::string> labels;
    auto *paths = new std::vector<std::string>();
    guint selected = 0;
    for (size_t i = 0; i < dirs.size(); ++i) {
      labels.push_back(dirs[i].path);
      paths->push_back(dirs[i].path);
      if (dirs[i].path == saved_dest) {
        selected = static_cast<guint>(i);
      }
    }
    labels.push_back(Translations::Tr("Other folder…"));
    paths->push_back({});
    std::vector<const char *> cstr;
    cstr.reserve(labels.size() + 1);
    for (const std::string &label : labels) {
      cstr.push_back(label.c_str());
    }
    cstr.push_back(nullptr);
    dest_drop = gtk_drop_down_new_from_strings(cstr.data());
    gtk_drop_down_set_selected(GTK_DROP_DOWN(dest_drop), selected);
    g_object_set_data_full(G_OBJECT(dest_drop), "dest-paths", paths, [](gpointer p) { delete static_cast<std::vector<std::string> *>(p); });
    g_object_set_data(G_OBJECT(dest_drop), "dest", dest);
    g_signal_connect(dest_drop, "notify::selected", G_CALLBACK((+[](GtkDropDown *drop, GParamSpec *, gpointer) {
                       auto *dest_paths = static_cast<std::vector<std::string> *>(g_object_get_data(G_OBJECT(drop), "dest-paths"));
                       auto *entry = GTK_EDITABLE(g_object_get_data(G_OBJECT(drop), "dest"));
                       const guint index = gtk_drop_down_get_selected(drop);
                       if (!dest_paths || !entry || index >= dest_paths->size() || (*dest_paths)[index].empty()) {
                         return;
                       }
                       gtk_editable_set_text(entry, (*dest_paths)[index].c_str());
                     })),
                     nullptr);
  }

  GtkWidget *move_btn = gtk_check_button_new_with_label(Translations::CStr("Move files instead of copying"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(move_btn), move || settings.BoolValue(OrganizeSettings::kMove, OrganizeSettings::kDefaultMove));
  GtkWidget *overwrite = gtk_check_button_new_with_label(Translations::CStr("Overwrite existing files"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(overwrite), settings.BoolValue(OrganizeSettings::kOverwrite, OrganizeSettings::kDefaultOverwrite));
  GtkWidget *replace_spaces = gtk_check_button_new_with_label(Translations::CStr("Replace spaces with underscores"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(replace_spaces),
                              settings.BoolValue(OrganizeSettings::kReplaceSpaces, OrganizeSettings::kDefaultReplaceSpaces));
  GtkWidget *albumcover = gtk_check_button_new_with_label(Translations::CStr("Copy album cover art"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(albumcover), settings.BoolValue(OrganizeSettings::kAlbumCover, OrganizeSettings::kDefaultAlbumCover));
  auto *owned_songs = new SongList(songs);
  GtkWidget *status = gtk_label_new(owned_songs->empty() ? Translations::CStr("Uses the current playlist as the source.")
                                                         : (std::to_string(owned_songs->size()) + " selected song(s).").c_str());
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  GtkWidget *preview = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(preview), TRUE);
  gtk_label_set_xalign(GTK_LABEL(preview), 0.0f);
  GtkWidget *preview_btn = gtk_button_new_with_label(Translations::CStr("Preview"));
  GtkWidget *run = gtk_button_new_with_label(Translations::CStr("Organize"));
  gtk_widget_add_css_class(run, "suggested-action");

  g_object_set_data(G_OBJECT(run), "buffer", buffer);
  g_object_set_data(G_OBJECT(run), "dest", dest);
  g_object_set_data(G_OBJECT(run), "move", move_btn);
  g_object_set_data(G_OBJECT(run), "overwrite", overwrite);
  g_object_set_data(G_OBJECT(run), "replace-spaces", replace_spaces);
  g_object_set_data(G_OBJECT(run), "albumcover", albumcover);
  g_object_set_data(G_OBJECT(run), "status", status);
  g_object_set_data(G_OBJECT(run), "parent", parent);
  g_object_set_data_full(G_OBJECT(run), "songs", owned_songs, [](gpointer p) { delete static_cast<SongList *>(p); });
  g_object_set_data(G_OBJECT(preview_btn), "songs", owned_songs);
  g_object_set_data(G_OBJECT(preview_btn), "buffer", buffer);
  g_object_set_data(G_OBJECT(preview_btn), "dest", dest);
  g_object_set_data(G_OBJECT(preview_btn), "replace-spaces", replace_spaces);
  g_object_set_data(G_OBJECT(preview_btn), "preview", preview);
  g_object_set_data(G_OBJECT(buffer), "error", format_error);
  g_object_set_data(G_OBJECT(buffer), "run", run);

  g_signal_connect(buffer, "changed", G_CALLBACK(+[](GtkTextBuffer *buf, gpointer) {
                     GtkTextIter start;
                     GtkTextIter end;
                     gtk_text_buffer_get_bounds(buf, &start, &end);
                     gchar *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
                     const std::string format = text ? text : "";
                     g_free(text);
                     OrganizeSyntaxHighlighter highlight;
                     highlight.Highlight(buf, format);
                     std::string error;
                     const bool valid = OrganizeFormatValidator::IsValid(format, &error);
                     gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(buf), "error")), valid ? "" : error.c_str());
                     gtk_widget_set_sensitive(GTK_WIDGET(g_object_get_data(G_OBJECT(buf), "run")), valid);
                   }),
                   nullptr);

  g_signal_connect(preview_btn, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *buf = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(button), "buffer"));
                     GtkTextIter start;
                     GtkTextIter end;
                     gtk_text_buffer_get_bounds(buf, &start, &end);
                     gchar *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
                     OrganizeFormat format(text ? text : "");
                     g_free(text);
                     format.set_replace_spaces(gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "replace-spaces"))));
                     const std::string dest_dir = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest")));
                     std::string preview_text;
                     auto *owned = static_cast<SongList *>(g_object_get_data(G_OBJECT(button), "songs"));
                     SongList songs = owned && !owned->empty() ? *owned
                                      : application->playlist_manager()->current() ? application->playlist_manager()->current()->songs()
                                                                                   : SongList{};
                     for (size_t i = 0; i < songs.size() && i < 8; ++i) {
                       preview_text += FileUtils::Join(dest_dir, format.GetFilenameForSong(songs[i])) + "\n";
                     }
                     if (songs.size() > 8) {
                       preview_text += "… " + std::to_string(songs.size() - 8) + " more";
                     }
                     gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "preview")), preview_text.c_str());
                   }),
                   app);
  g_signal_connect(run, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *buf = GTK_TEXT_BUFFER(g_object_get_data(G_OBJECT(button), "buffer"));
                     GtkTextIter start;
                     GtkTextIter end;
                     gtk_text_buffer_get_bounds(buf, &start, &end);
                     gchar *text = gtk_text_buffer_get_text(buf, &start, &end, FALSE);
                     const std::string format_text = text ? text : "";
                     g_free(text);
                     std::string error;
                     if (!OrganizeFormatValidator::IsValid(format_text, &error)) {
                       gtk_label_set_text(GTK_LABEL(g_object_get_data(G_OBJECT(button), "status")), error.c_str());
                       return;
                     }
                     OrganizeFormat format(format_text);
                     format.set_replace_spaces(gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "replace-spaces"))));
                     const std::string dest_dir = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest")));
                     Organize::Options options;
                     options.move = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "move")));
                     options.overwrite = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "overwrite")));
                     options.albumcover = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "albumcover")));
                     Settings persist;
                     persist.BeginGroup(OrganizeSettings::kSettingsGroup);
                     persist.SetValue(OrganizeSettings::kFormat, format_text);
                     persist.SetValue(OrganizeSettings::kDestination, dest_dir);
                     persist.SetBoolValue(OrganizeSettings::kMove, options.move);
                     persist.SetBoolValue(OrganizeSettings::kOverwrite, options.overwrite);
                     persist.SetBoolValue(OrganizeSettings::kReplaceSpaces, format.replace_spaces());
                     persist.SetBoolValue(OrganizeSettings::kAlbumCover, options.albumcover);
                     persist.Sync();
                     auto *owned = static_cast<SongList *>(g_object_get_data(G_OBJECT(button), "songs"));
                     SongList songs = owned && !owned->empty() ? *owned
                                      : application->playlist_manager()->current() ? application->playlist_manager()->current()->songs()
                                                                                   : SongList{};
                     class Organize organize;
                     const auto errors = organize.Copy(songs, dest_dir, format, options);
                     GtkWidget *status_label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "status"));
                     if (application && application->collection()) {
                       application->collection()->IncrementalScan();
                     }
                     if (errors.empty()) {
                       gtk_label_set_text(GTK_LABEL(status_label), Translations::CStr("Organize finished."));
                       return;
                     }
                     OrganizeErrorDialog::Show(GTK_WINDOW(g_object_get_data(G_OBJECT(button), "parent")), errors);
                     gtk_label_set_text(GTK_LABEL(status_label), (std::to_string(errors.size()) + " file(s) failed.").c_str());
                   }),
                   app);

  gtk_box_append(GTK_BOX(box), format_header);
  gtk_box_append(GTK_BOX(box), format_view);
  gtk_box_append(GTK_BOX(box), format_error);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Destination")));
  if (dest_drop) {
    gtk_box_append(GTK_BOX(box), dest_drop);
  }
  gtk_box_append(GTK_BOX(box), dest_row);
  gtk_box_append(GTK_BOX(box), move_btn);
  gtk_box_append(GTK_BOX(box), overwrite);
  gtk_box_append(GTK_BOX(box), replace_spaces);
  gtk_box_append(GTK_BOX(box), albumcover);
  gtk_box_append(GTK_BOX(box), preview_btn);
  gtk_box_append(GTK_BOX(box), preview);
  gtk_box_append(GTK_BOX(box), run);
  gtk_box_append(GTK_BOX(box), status);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
