#include "globalshortcuts/globalshortcutgrabber.h"

#include "globalshortcuts/globalshortcutgrab.h"

#include <adwaita.h>

void GlobalShortcutGrabber::Show(GtkWindow *parent, const std::function<void(const std::string &)> &callback, const std::string &action) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(GlobalShortcutGrab::WindowTitle(), GlobalShortcutGrab::Prompt(action).c_str()));
  GtkWidget *label = gtk_label_new(GlobalShortcutGrab::WaitingLabel());
  gtk_label_set_use_markup(GTK_LABEL(label), TRUE);
  gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
  adw_alert_dialog_set_extra_child(dialog, label);
  adw_alert_dialog_add_response(dialog, "cancel", "Cancel");
  auto *cb = new std::function<void(const std::string &)>(callback);
  g_object_set_data(G_OBJECT(dialog), "label", label);
  g_object_set_data(G_OBJECT(dialog), "callback", cb);
  g_object_set_data(G_OBJECT(dialog), "accepted", GINT_TO_POINTER(0));
  g_object_set_data_full(G_OBJECT(dialog), "accel", g_strdup(""), g_free);
  GtkEventController *keys = gtk_event_controller_key_new();
  g_signal_connect(keys, "key-pressed",
                   G_CALLBACK(+[](GtkEventControllerKey *, guint keyval, guint, GdkModifierType state, gpointer data) -> gboolean {
                     AdwAlertDialog *alert = ADW_ALERT_DIALOG(data);
                     GtkWidget *lab = GTK_WIDGET(g_object_get_data(G_OBJECT(alert), "label"));
                     gchar *name = gtk_accelerator_name(keyval, state);
                     const std::string accel = name ? name : "";
                     g_free(name);
                     g_object_set_data_full(G_OBJECT(alert), "accel", g_strdup(accel.c_str()), g_free);
                     if (lab) {
                       const std::string markup = GlobalShortcutGrab::PreviewMarkup(accel);
                       gtk_label_set_markup(GTK_LABEL(lab), markup.empty() ? GlobalShortcutGrab::WaitingLabel() : markup.c_str());
                     }
                     if (!GlobalShortcutGrab::ShouldAccept(keyval)) {
                       return TRUE;
                     }
                     g_object_set_data(G_OBJECT(alert), "accepted", GINT_TO_POINTER(1));
                     adw_dialog_close(ADW_DIALOG(alert));
                     return TRUE;
                   }),
                   dialog);
  gtk_widget_add_controller(GTK_WIDGET(dialog), keys);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *, gpointer data) {
                     auto *fn = static_cast<std::function<void(const std::string &)> *>(data);
                     const bool accepted = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(alert), "accepted")) != 0;
                     const char *accel = static_cast<const char *>(g_object_get_data(G_OBJECT(alert), "accel"));
                     if (accepted && fn) {
                       (*fn)(accel ? accel : "");
                     }
                     delete fn;
                   }),
                   cb);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
