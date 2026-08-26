#include "covermanager/albumcoverexportdialog.h"

#include "core/application.h"
#include "dialogs/dialoghelpers.h"
#include "translations/translations.h"
#include "utilities/fileutils.h"

#include <adwaita.h>
#include <glib/gstdio.h>

#include <string>

using DialogHelpers::SafeFolderName;

namespace {

struct CoverExportJob {
  Application *app = nullptr;
  std::string filename;
  bool overwrite = true;
};

}  // namespace

void AlbumCoverExportDialog::Show(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Export album covers"));
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
  GtkWidget *status = gtk_label_new(Translations::CStr("Exports each album’s artwork into Artist - Album folders."));
  gtk_label_set_wrap(GTK_LABEL(status), TRUE);
  GtkWidget *export_btn = gtk_button_new_with_label(Translations::CStr("Choose folder…"));
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
