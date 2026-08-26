#include "organize/organizedialog.h"

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

void OrganizeDialog::Show(GtkWindow *parent, Application *app, const SongList &songs) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Organize files"));
  adw_dialog_set_content_width(dialog, 520);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 12);
  gtk_widget_set_margin_start(box, 18);
  gtk_widget_set_margin_end(box, 18);
  gtk_widget_set_margin_top(box, 18);
  gtk_widget_set_margin_bottom(box, 18);

  Settings settings;
  settings.BeginGroup("Organize");
  const std::string saved_format = settings.Value("format", "%albumartist/%album/{%track - }%title");
  const std::string saved_dest = settings.Value("destination", g_get_user_special_dir(G_USER_DIRECTORY_MUSIC)
                                                                   ? g_get_user_special_dir(G_USER_DIRECTORY_MUSIC)
                                                                   : g_get_home_dir());

  GtkWidget *format_view = gtk_text_view_new();
  gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(format_view), GTK_WRAP_WORD_CHAR);
  gtk_widget_set_size_request(format_view, -1, 56);
  GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(format_view));
  OrganizeSyntaxHighlighter highlighter;
  highlighter.Apply(buffer, saved_format);
  GtkWidget *format_error = gtk_label_new("");
  gtk_widget_add_css_class(format_error, "error");
  gtk_label_set_xalign(GTK_LABEL(format_error), 0.0f);
  GtkWidget *dest = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(dest), saved_dest.c_str());
  GtkWidget *move = gtk_check_button_new_with_label(Translations::CStr("Move files instead of copying"));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(move), settings.BoolValue("move", false));
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
  g_object_set_data(G_OBJECT(run), "move", move);
  g_object_set_data(G_OBJECT(run), "status", status);
  g_object_set_data(G_OBJECT(run), "parent", parent);
  g_object_set_data_full(G_OBJECT(run), "songs", owned_songs, [](gpointer p) { delete static_cast<SongList *>(p); });
  g_object_set_data(G_OBJECT(preview_btn), "songs", owned_songs);
  g_object_set_data(G_OBJECT(preview_btn), "buffer", buffer);
  g_object_set_data(G_OBJECT(preview_btn), "dest", dest);
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
                     const std::string dest_dir = gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest")));
                     const bool move_files = gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "move")));
                     Settings persist;
                     persist.BeginGroup("Organize");
                     persist.SetValue("format", format_text);
                     persist.SetValue("destination", dest_dir);
                     persist.SetBoolValue("move", move_files);
                     persist.Sync();
                     auto *owned = static_cast<SongList *>(g_object_get_data(G_OBJECT(button), "songs"));
                     SongList songs = owned && !owned->empty() ? *owned
                                      : application->playlist_manager()->current() ? application->playlist_manager()->current()->songs()
                                                                                   : SongList{};
                     class Organize organize;
                     const auto errors = organize.Copy(songs, dest_dir, format, move_files);
                     GtkWidget *status_label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "status"));
                     if (errors.empty()) {
                       gtk_label_set_text(GTK_LABEL(status_label), Translations::CStr("Organize finished."));
                       return;
                     }
                     OrganizeErrorDialog::Show(GTK_WINDOW(g_object_get_data(G_OBJECT(button), "parent")), errors);
                     gtk_label_set_text(GTK_LABEL(status_label), (std::to_string(errors.size()) + " file(s) failed.").c_str());
                   }),
                   app);

  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Filename format")));
  gtk_box_append(GTK_BOX(box), format_view);
  gtk_box_append(GTK_BOX(box), format_error);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr("Destination")));
  gtk_box_append(GTK_BOX(box), dest);
  gtk_box_append(GTK_BOX(box), move);
  gtk_box_append(GTK_BOX(box), preview_btn);
  gtk_box_append(GTK_BOX(box), preview);
  gtk_box_append(GTK_BOX(box), run);
  gtk_box_append(GTK_BOX(box), status);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
