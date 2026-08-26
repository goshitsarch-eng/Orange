#include "organize/organizedialog.h"

#include "core/application.h"
#include "organize/organize.h"
#include "utilities/fileutils.h"

#include <adwaita.h>

void OrganizeDialog::Show(GtkWindow *parent, Application *app) {
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
