#include "dialogs/errordialog.h"

#include "dialogs/errordialoglabels.h"
#include "translations/translations.h"

#include <adwaita.h>

void ErrorDialog::Show(GtkWindow *parent, const std::string &message) {
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new(Translations::CStr(ErrorDialogLabels::Title()), message.c_str()));
  adw_alert_dialog_add_response(dialog, "ok", Translations::CStr("OK"));
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
