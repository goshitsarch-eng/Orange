#include "dialogs/userpassdialog.h"

#include <adwaita.h>

void UserPassDialog::Show(GtkWindow *parent, const std::string &service, const std::function<void(const std::string &, const std::string &)> &callback) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(("Sign in to " + service).c_str(), "Enter username and password or token."));
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  GtkWidget *user = gtk_entry_new();
  gtk_entry_set_placeholder_text(GTK_ENTRY(user), "Username");
  GtkWidget *pass = gtk_password_entry_new();
  gtk_password_entry_set_show_peek_icon(GTK_PASSWORD_ENTRY(pass), TRUE);
  gtk_box_append(GTK_BOX(box), user);
  gtk_box_append(GTK_BOX(box), pass);
  adw_alert_dialog_set_extra_child(dialog, box);
  adw_alert_dialog_add_responses(dialog, "cancel", "Cancel", "login", "Sign in", nullptr);
  adw_alert_dialog_set_response_appearance(dialog, "login", ADW_RESPONSE_SUGGESTED);
  auto *cb = new std::function<void(const std::string &, const std::string &)>(callback);
  g_object_set_data(G_OBJECT(dialog), "user", user);
  g_object_set_data(G_OBJECT(dialog), "pass", pass);
  g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *response, gpointer data) {
                     auto *fn = static_cast<std::function<void(const std::string &, const std::string &)> *>(data);
                     if (g_strcmp0(response, "login") == 0) {
                       (*fn)(gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "user"))),
                             gtk_editable_get_text(GTK_EDITABLE(g_object_get_data(G_OBJECT(alert), "pass"))));
                     }
                     delete fn;
                   }),
                   cb);
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
