#include "dialogs/messagedialog.h"

#include "translations/translations.h"

#include <adwaita.h>

void MessageDialog::Show(GtkWindow *parent, const std::string &title, const std::string &message) {
  Show(parent, title, message, {}, false, {});
}

void MessageDialog::Show(GtkWindow *parent, const std::string &title, const std::string &message, const std::string &checkbox_label,
                         bool checkbox_checked, const std::function<void(bool checked)> &closed) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(title.c_str()), Translations::CStr(message.c_str())));
  adw_alert_dialog_add_response(dialog, "ok", Translations::CStr("OK"));
  GtkWidget *check = nullptr;
  if (!checkbox_label.empty()) {
    check = gtk_check_button_new_with_label(Translations::CStr(checkbox_label.c_str()));
    gtk_check_button_set_active(GTK_CHECK_BUTTON(check), checkbox_checked ? TRUE : FALSE);
    adw_alert_dialog_set_extra_child(dialog, check);
  }
  if (closed) {
    auto *fn = new std::function<void(bool)>(closed);
    g_object_set_data_full(G_OBJECT(dialog), "message-closed", fn, [](gpointer p) { delete static_cast<std::function<void(bool)> *>(p); });
    if (check) {
      g_object_set_data(G_OBJECT(dialog), "message-check", check);
    }
    g_signal_connect(dialog, "response", G_CALLBACK(+[](AdwAlertDialog *alert, const char *, gpointer) {
                       auto *callback = static_cast<std::function<void(bool)> *>(g_object_get_data(G_OBJECT(alert), "message-closed"));
                       auto *box = GTK_CHECK_BUTTON(g_object_get_data(G_OBJECT(alert), "message-check"));
                       if (callback && *callback) {
                         (*callback)(box && gtk_check_button_get_active(box));
                       }
                     }),
                     nullptr);
  }
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
