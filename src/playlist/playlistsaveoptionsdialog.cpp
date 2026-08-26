#include "playlist/playlistsaveoptionsdialog.h"

#include "playlist/playlistsaveoptions.h"
#include "translations/translations.h"

#include <adwaita.h>

const char *PlaylistSaveOptionsDialog::Label(PathType type) {
  switch (type) {
    case PathType::Absolute:
      return Translations::CStr("Absolute paths");
    case PathType::Relative:
      return Translations::CStr("Relative paths");
    case PathType::Ask_User:
      return Translations::CStr("Ask every time");
    case PathType::Automatic:
    default:
      return Translations::CStr("Automatic");
  }
}

void PlaylistSaveOptionsDialog::Show(GtkWindow *parent, const std::function<void(PathType)> &callback) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr(PlaylistSaveOptions::Title()));
  adw_dialog_set_content_width(dialog, 360);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  const char *labels[] = {Translations::CStr(PlaylistSaveOptions::DialogLabel(PlaylistSaveOptions::kDialogChoices[0])),
                          Translations::CStr(PlaylistSaveOptions::DialogLabel(PlaylistSaveOptions::kDialogChoices[1])),
                          Translations::CStr(PlaylistSaveOptions::DialogLabel(PlaylistSaveOptions::kDialogChoices[2])), nullptr};
  GtkWidget *drop = gtk_drop_down_new_from_strings(labels);
  GtkWidget *remember = gtk_check_button_new_with_label(Translations::CStr(PlaylistSaveOptions::RememberLabel()));
  GtkWidget *hint = gtk_label_new(Translations::CStr(PlaylistSaveOptions::Hint()));
  gtk_widget_add_css_class(hint, "dim-label");
  gtk_label_set_wrap(GTK_LABEL(hint), TRUE);
  gtk_label_set_xalign(GTK_LABEL(hint), 0.0f);
  GtkWidget *save = gtk_button_new_with_label(Translations::CStr("Save"));
  gtk_widget_add_css_class(save, "suggested-action");
  auto *cb = new std::function<void(PathType)>(callback);
  g_object_set_data_full(G_OBJECT(dialog), "callback", cb, [](gpointer p) { delete static_cast<std::function<void(PathType)> *>(p); });
  g_object_set_data(G_OBJECT(save), "dialog", dialog);
  g_object_set_data(G_OBJECT(save), "drop", drop);
  g_object_set_data(G_OBJECT(save), "remember", remember);
  g_signal_connect(save, "clicked", G_CALLBACK(+[](GtkButton *button, gpointer data) {
                     auto *fn = static_cast<std::function<void(PathType)> *>(data);
                     GtkWidget *dlg = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "dialog"));
                     const guint selected = gtk_drop_down_get_selected(GTK_DROP_DOWN(g_object_get_data(G_OBJECT(button), "drop")));
                     const bool remember_choice =
                         gtk_check_button_get_active(GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(button), "remember")));
                     const PathType type = PlaylistSaveOptions::PathFromIndex(static_cast<int>(selected));
                     PlaylistSaveOptions::MaybeRemember(remember_choice, type);
                     if (fn) {
                       (*fn)(type);
                     }
                     adw_dialog_close(ADW_DIALOG(dlg));
                   }),
                   cb);
  gtk_box_append(GTK_BOX(box), gtk_label_new(Translations::CStr(PlaylistSaveOptions::PathsLabel())));
  gtk_box_append(GTK_BOX(box), drop);
  gtk_box_append(GTK_BOX(box), remember);
  gtk_box_append(GTK_BOX(box), hint);
  gtk_box_append(GTK_BOX(box), save);
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
