#include "globalshortcuts/globalshortcutgrabber.h"

#include "globalshortcuts/globalshortcutgrab.h"

#include <adwaita.h>

namespace {

void StoreAccel(AdwAlertDialog *alert, const std::string &accel) {
  g_object_set_data_full(G_OBJECT(alert), "accel", g_strdup(accel.c_str()), g_free);
}

const char *StoredAccel(AdwAlertDialog *alert) {
  const char *accel = static_cast<const char *>(g_object_get_data(G_OBJECT(alert), "accel"));
  return accel ? accel : "";
}

void UpdatePreview(AdwAlertDialog *alert) {
  GtkWidget *lab = GTK_WIDGET(g_object_get_data(G_OBJECT(alert), "label"));
  if (!lab) {
    return;
  }
  const std::string markup = GlobalShortcutGrab::PreviewMarkup(StoredAccel(alert));
  gtk_label_set_markup(GTK_LABEL(lab), markup.empty() ? GlobalShortcutGrab::WaitingLabel() : markup.c_str());
}

void TryDismiss(AdwAlertDialog *alert, bool accepted) {
  if (!accepted && !GlobalShortcutGrab::ShouldDismissOnCancel(StoredAccel(alert))) {
    return;
  }
  g_object_set_data(G_OBJECT(alert), "accepted", GINT_TO_POINTER(accepted ? 1 : 0));
  adw_dialog_force_close(ADW_DIALOG(alert));
}

}  // namespace

void GlobalShortcutGrabber::Show(GtkWindow *parent, const std::function<void(const std::string &)> &callback, const std::string &action) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(GlobalShortcutGrab::WindowTitle(), GlobalShortcutGrab::Prompt(action).c_str()));
  adw_dialog_set_can_close(ADW_DIALOG(dialog), FALSE);
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  GtkWidget *label = gtk_label_new(GlobalShortcutGrab::WaitingLabel());
  gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  GtkWidget *cancel = gtk_button_new_with_label("Cancel");
  gtk_widget_set_halign(cancel, GTK_ALIGN_CENTER);
  gtk_box_append(GTK_BOX(box), label);
  gtk_box_append(GTK_BOX(box), cancel);
  adw_alert_dialog_set_extra_child(dialog, box);
  auto *cb = new std::function<void(const std::string &)>(callback);
  g_object_set_data(G_OBJECT(dialog), "label", label);
  g_object_set_data(G_OBJECT(dialog), "callback", cb);
  g_object_set_data(G_OBJECT(dialog), "accepted", GINT_TO_POINTER(0));
  StoreAccel(dialog, {});
  g_signal_connect(cancel, "clicked", G_CALLBACK(+[](GtkButton *, gpointer data) { TryDismiss(ADW_ALERT_DIALOG(data), false); }), dialog);
  GtkEventController *keys = gtk_event_controller_key_new();
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK((+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     AdwAlertDialog *alert = ADW_ALERT_DIALOG(data);
                     if (keyval == GDK_KEY_Escape) {
                       TryDismiss(alert, false);
                       return TRUE;
                     }
                     gchar *name = gtk_accelerator_name(keyval, state);
                     StoreAccel(alert, name ? name : "");
                     g_free(name);
                     UpdatePreview(alert);
                     if (!GlobalShortcutGrab::ShouldAccept(keyval)) {
                       return TRUE;
                     }
                     TryDismiss(alert, true);
                     return TRUE;
                   })),
                   dialog);
  gtk_widget_add_controller(GTK_WIDGET(dialog), keys);
  g_signal_connect(dialog, "closed", G_CALLBACK(+[](AdwDialog *closed, gpointer data) {
                     auto *fn = static_cast<std::function<void(const std::string &)> *>(data);
                     AdwAlertDialog *alert = ADW_ALERT_DIALOG(closed);
                     const bool accepted = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(alert), "accepted")) != 0;
                     if (accepted && fn) {
                       (*fn)(StoredAccel(alert));
                     }
                     delete fn;
                   }),
                   cb);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
