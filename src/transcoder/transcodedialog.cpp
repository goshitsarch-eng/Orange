#include "transcoder/transcodedialog.h"

#include "core/application.h"
#include "transcoder/transcoder.h"
#include "transcoder/transcoderoptionsdialog.h"
#include "utilities/fileutils.h"

#include <adwaita.h>

void TranscodeDialog::Show(GtkWindow *parent, Application *app) {
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
  GtkWidget *options = gtk_button_new_with_label("Format options…");
  GtkWidget *dest = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(dest), g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) ? g_get_user_special_dir(G_USER_DIRECTORY_MUSIC) : g_get_home_dir());
  gtk_box_append(GTK_BOX(box), gtk_label_new("Output format"));
  gtk_box_append(GTK_BOX(box), formats);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Quality"));
  gtk_box_append(GTK_BOX(box), quality);
  gtk_box_append(GTK_BOX(box), options);
  gtk_box_append(GTK_BOX(box), gtk_label_new("Destination folder"));
  gtk_box_append(GTK_BOX(box), dest);
  GtkWidget *log = gtk_label_new("");
  gtk_label_set_wrap(GTK_LABEL(log), TRUE);
  gtk_box_append(GTK_BOX(box), log);
  GtkWidget *start = gtk_button_new_with_label("Start");
  g_object_set_data(G_OBJECT(start), "formats", formats);
  g_object_set_data(G_OBJECT(start), "dest", dest);
  g_object_set_data(G_OBJECT(start), "log", log);
  g_object_set_data(G_OBJECT(start), "quality", quality);
  g_object_set_data(G_OBJECT(options), "formats", formats);
  g_object_set_data(G_OBJECT(options), "quality", quality);
  g_signal_connect(options, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer) {
                     auto *formats_w = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "formats"));
                     auto *quality_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "quality"));
                     const auto format = static_cast<Transcoder::Format>(gtk_drop_down_get_selected(formats_w));
                     TranscoderOptionsDialog::Show(nullptr, format, [quality_w](int value) {
                       gtk_spin_button_set_value(quality_w, value);
                     });
                   }),
                   nullptr);
  g_signal_connect(start, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *formats_w = GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "formats"));
                     auto *dest_w = GTK_EDITABLE(g_object_get_data(G_OBJECT(button), "dest"));
                     auto *log_w = GTK_LABEL(g_object_get_data(G_OBJECT(button), "log"));
                     const auto format = static_cast<Transcoder::Format>(gtk_drop_down_get_selected(formats_w));
                     const std::string dest_dir = gtk_editable_get_text(dest_w);
                     auto *quality_w = GTK_SPIN_BUTTON(g_object_get_data(G_OBJECT(button), "quality"));
                     application->transcoder()->Cancel();
                     application->transcoder()->set_quality(static_cast<int>(gtk_spin_button_get_value(quality_w)));
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
                     std::string text = std::to_string(count) + " jobs as " + Transcoder::FormatName(format);
                     if (!application->transcoder()->log().empty()) {
                       text += "\n" + application->transcoder()->log().back();
                     }
                     gtk_label_set_text(log_w, text.c_str());
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), start);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
