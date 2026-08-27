#include "covermanager/albumcoverexportdialog.h"

#include "collection/collectionlibrary.h"
#include "core/application.h"
#include "core/settings.h"
#include "covermanager/albumcoverexport.h"
#include "covermanager/albumcoverexporter.h"
#include "covermanager/albumcoverexportlabels.h"
#include "covermanager/covermanagerexportscope.h"
#include "translations/translations.h"

#include <adwaita.h>

#include <string>

namespace {

struct CoverExportWidgets {
  GtkWidget *filename = nullptr;
  GtkWidget *downloaded = nullptr;
  GtkWidget *embedded = nullptr;
  GtkWidget *overwrite_none = nullptr;
  GtkWidget *overwrite_all = nullptr;
  GtkWidget *overwrite_smaller = nullptr;
  GtkWidget *force_size = nullptr;
  GtkWidget *width = nullptr;
  GtkWidget *height = nullptr;
};

AlbumCoverExport::DialogResult CollectResult(const CoverExportWidgets &widgets) {
  AlbumCoverExport::DialogResult result = AlbumCoverExportLabels::Defaults();
  const char *filename = gtk_editable_get_text(GTK_EDITABLE(widgets.filename));
  result.filename = filename && filename[0] ? filename : AlbumCoverExportLabels::DefaultFilename();
  result.export_downloaded = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets.downloaded));
  result.export_embedded = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets.embedded));
  if (gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets.overwrite_all))) {
    result.overwrite = AlbumCoverExport::OverwriteMode::All;
  } else if (gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets.overwrite_smaller))) {
    result.overwrite = AlbumCoverExport::OverwriteMode::Smaller;
  } else {
    result.overwrite = AlbumCoverExport::OverwriteMode::None;
  }
  result.forcesize = gtk_check_button_get_active(GTK_CHECK_BUTTON(widgets.force_size));
  result.width = static_cast<int>(g_ascii_strtoll(gtk_editable_get_text(GTK_EDITABLE(widgets.width)), nullptr, 10));
  result.height = static_cast<int>(g_ascii_strtoll(gtk_editable_get_text(GTK_EDITABLE(widgets.height)), nullptr, 10));
  return result;
}

}  // namespace

void AlbumCoverExportDialog::Show(GtkWindow *parent, Application *app, const SongList &songs) {
  Settings settings;
  const AlbumCoverExport::DialogResult saved = AlbumCoverExportLabels::FromSettings(&settings);

  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr(AlbumCoverExportLabels::Title()));
  adw_dialog_set_content_width(dialog, 480);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);

  GtkWidget *output_label = gtk_label_new(Translations::CStr(AlbumCoverExportLabels::Output()));
  gtk_widget_set_halign(output_label, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), output_label);
  GtkWidget *prompt = gtk_label_new(Translations::CStr(AlbumCoverExportLabels::FilenamePrompt()));
  gtk_label_set_wrap(GTK_LABEL(prompt), TRUE);
  gtk_label_set_xalign(GTK_LABEL(prompt), 0.0f);
  gtk_box_append(GTK_BOX(box), prompt);

  auto *widgets = new CoverExportWidgets();
  widgets->filename = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(widgets->filename), saved.filename.c_str());
  gtk_box_append(GTK_BOX(box), widgets->filename);

  widgets->downloaded = gtk_check_button_new_with_label(Translations::CStr(AlbumCoverExportLabels::ExportDownloaded()));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->downloaded), saved.export_downloaded ? TRUE : FALSE);
  gtk_box_append(GTK_BOX(box), widgets->downloaded);

  widgets->embedded = gtk_check_button_new_with_label(Translations::CStr(AlbumCoverExportLabels::ExportEmbedded()));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->embedded), saved.export_embedded ? TRUE : FALSE);
  gtk_box_append(GTK_BOX(box), widgets->embedded);

  GtkWidget *existing = gtk_label_new(Translations::CStr(AlbumCoverExportLabels::ExistingCovers()));
  gtk_widget_set_halign(existing, GTK_ALIGN_START);
  gtk_box_append(GTK_BOX(box), existing);

  widgets->overwrite_none = gtk_check_button_new_with_label(Translations::CStr(AlbumCoverExportLabels::DoNotOverwrite()));
  widgets->overwrite_all = gtk_check_button_new_with_label(Translations::CStr(AlbumCoverExportLabels::OverwriteAll()));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(widgets->overwrite_all), GTK_CHECK_BUTTON(widgets->overwrite_none));
  widgets->overwrite_smaller = gtk_check_button_new_with_label(Translations::CStr(AlbumCoverExportLabels::OverwriteSmaller()));
  gtk_check_button_set_group(GTK_CHECK_BUTTON(widgets->overwrite_smaller), GTK_CHECK_BUTTON(widgets->overwrite_none));
  const int overwrite = AlbumCoverExportLabels::OverwriteRadioIndex(saved.overwrite);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->overwrite_none), overwrite == 0 ? TRUE : FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->overwrite_all), overwrite == 1 ? TRUE : FALSE);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->overwrite_smaller), overwrite == 2 ? TRUE : FALSE);
  gtk_box_append(GTK_BOX(box), widgets->overwrite_none);
  gtk_box_append(GTK_BOX(box), widgets->overwrite_all);
  gtk_box_append(GTK_BOX(box), widgets->overwrite_smaller);

  widgets->force_size = gtk_check_button_new_with_label(Translations::CStr(AlbumCoverExportLabels::ScaleSize()));
  gtk_check_button_set_active(GTK_CHECK_BUTTON(widgets->force_size), saved.forcesize ? TRUE : FALSE);
  gtk_box_append(GTK_BOX(box), widgets->force_size);

  GtkWidget *size_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 8);
  gtk_box_append(GTK_BOX(size_row), gtk_label_new(Translations::CStr(AlbumCoverExportLabels::Size())));
  widgets->width = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(widgets->width), saved.width > 0 ? std::to_string(saved.width).c_str() : "");
  gtk_editable_set_max_width_chars(GTK_EDITABLE(widgets->width), 4);
  widgets->height = gtk_entry_new();
  gtk_editable_set_text(GTK_EDITABLE(widgets->height), saved.height > 0 ? std::to_string(saved.height).c_str() : "");
  gtk_editable_set_max_width_chars(GTK_EDITABLE(widgets->height), 4);
  gtk_box_append(GTK_BOX(size_row), widgets->width);
  gtk_box_append(GTK_BOX(size_row), gtk_label_new("×"));
  gtk_box_append(GTK_BOX(size_row), widgets->height);
  gtk_box_append(GTK_BOX(size_row), gtk_label_new(Translations::CStr(AlbumCoverExportLabels::Pixel())));
  gtk_widget_set_sensitive(size_row, AlbumCoverExportLabels::ForceSizeEnabled(saved.forcesize) ? TRUE : FALSE);
  g_object_set_data(G_OBJECT(widgets->force_size), "size-row", size_row);
  g_signal_connect(widgets->force_size, "toggled", G_CALLBACK(+[](GtkCheckButton *button, gpointer) {
                     gtk_widget_set_sensitive(GTK_WIDGET(g_object_get_data(G_OBJECT(button), "size-row")),
                                              gtk_check_button_get_active(button));
                   }),
                   nullptr);
  gtk_box_append(GTK_BOX(box), size_row);

  GtkWidget *export_btn = gtk_button_new_with_label(Translations::CStr(AlbumCoverExportLabels::Export()));
  gtk_widget_add_css_class(export_btn, "suggested-action");
  g_object_set_data_full(G_OBJECT(export_btn), "widgets", widgets, [](gpointer p) { delete static_cast<CoverExportWidgets *>(p); });
  g_object_set_data_full(G_OBJECT(export_btn), "songs", new SongList(songs), [](gpointer p) { delete static_cast<SongList *>(p); });
  g_signal_connect(export_btn, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *application = static_cast<Application *>(data);
                     auto *widgets = static_cast<CoverExportWidgets *>(g_object_get_data(G_OBJECT(button), "widgets"));
                     auto *export_songs = static_cast<SongList *>(g_object_get_data(G_OBJECT(button), "songs"));
                     if (!application || !widgets) {
                       return;
                     }
                     const AlbumCoverExport::DialogResult result = CollectResult(*widgets);
                     Settings settings;
                     AlbumCoverExportLabels::ApplyToSettings(&settings, result);
                     if (!export_songs || export_songs->empty()) {
                       AdwAlertDialog *empty = ADW_ALERT_DIALOG(
                           adw_alert_dialog_new(CoverManagerExportScope::FinishedTitle(), CoverManagerExportScope::NoCoversText()));
                       adw_alert_dialog_add_response(empty, "ok", "OK");
                       adw_dialog_present(ADW_DIALOG(empty), nullptr);
                       return;
                     }
                     AlbumCoverExporter exporter(application->tagreader());
                     exporter.SetDialogResult(result);
                     exporter.SetCoverTypes(AlbumCoverExportLabels::TypesFor(result));
                     for (const Song &song : *export_songs) {
                       exporter.AddExportRequest(song);
                     }
                     exporter.StartExporting();
                     const std::string body = CoverManagerExportScope::FinishedBody(exporter.exported(), exporter.skipped());
                     AdwAlertDialog *done = ADW_ALERT_DIALOG(adw_alert_dialog_new(CoverManagerExportScope::FinishedTitle(), body.c_str()));
                     adw_alert_dialog_add_response(done, "ok", "OK");
                     adw_dialog_present(ADW_DIALOG(done), nullptr);
                   }),
                   app);
  gtk_box_append(GTK_BOX(box), export_btn);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, parent ? GTK_WIDGET(parent) : nullptr);
}
