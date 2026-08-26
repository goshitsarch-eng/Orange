#include "organize/organizeerrordialog.h"

#include <adwaita.h>

void OrganizeErrorDialog::Show(GtkWindow *parent, const std::vector<Organize::Error> &errors) {
  std::string body;
  for (const Organize::Error &error : errors) {
    if (!body.empty()) {
      body += "\n";
    }
    body += error.song.empty() ? error.message : error.song + ": " + error.message;
  }
  AdwDialog *dialog = adw_alert_dialog_new("Organize failed", body.empty() ? "Unknown error" : body.c_str());
  adw_alert_dialog_add_response(ADW_ALERT_DIALOG(dialog), "ok", "OK");
  adw_dialog_present(dialog, parent ? GTK_WIDGET(parent) : nullptr);
}
