#include "dialogs/errordialog.h"

#include <adwaita.h>

void ErrorDialog::Show(GtkWindow *parent, const std::string &message) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Error", message.c_str()));
  adw_alert_dialog_add_response(dialog, "ok", "OK");
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
