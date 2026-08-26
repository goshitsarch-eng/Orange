#include "playlist/playlistsaveoptionsdialog.h"

#include <adwaita.h>

const char *PlaylistSaveOptionsDialog::Label(PathType type) {
  switch (type) {
    case PathType::Relative:
      return "Relative paths";
    case PathType::Absolute:
      return "Absolute paths";
    case PathType::Automatic:
    default:
      return "Automatic";
  }
}

void PlaylistSaveOptionsDialog::Show(GtkWindow *parent, const std::function<void(PathType)> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Playlist save options");
  adw_dialog_set_content_width(dialog, 360);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  static const char *labels[] = {"Automatic", "Relative paths", "Absolute paths", nullptr};
  GtkWidget *drop = gtk_drop_down_new_from_strings(labels);
  GtkWidget *save = gtk_button_new_with_label("Save");
  gtk_widget_add_css_class(save, "suggested-action");
  auto *cb = new std::function<void(PathType)>(callback);
  g_object_set_data_full(G_OBJECT(dialog), "callback", cb, [](gpointer p) { delete static_cast<std::function<void(PathType)> *>(p); });
  g_object_set_data(G_OBJECT(save), "dialog", dialog);
  g_object_set_data(G_OBJECT(save), "drop", drop);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *fn = static_cast<std::function<void(PathType)> *>(data);
                     GtkWidget *dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "dialog"));
                     const guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "drop")));
                     if (fn) {
                       (*fn)(static_cast<PathType>(selected));
                     }
                     adw_dialog_close(ADW_DIALOG(dlg));
                   }),
                   cb);
  gtk_box_append(GTK_BOX(box), gtk_label_new("File path style"));
  gtk_box_append(GTK_BOX(box), drop);
  gtk_box_append(GTK_BOX(box), save);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
