#include "globalshortcuts/globalshortcutgrabber.h"

#include <adwaita.h>

void GlobalShortcutGrabber::Show(GtkWindow *parent, const std::function<void(const std::string &)> &callback) {
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
