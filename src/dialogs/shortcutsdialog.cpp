#include "dialogs/shortcutsdialog.h"

#include "translations/translations.h"

#include <adwaita.h>

void ShortcutsDialog::Show(GtkWindow *parent) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Keyboard shortcuts"));
  GtkWidget *label = gtk_label_new(
      "Space  Play/Pause\nCtrl+Right  Next\nCtrl+Left  Previous\nCtrl+Up  Volume up\nCtrl+Down  Volume down\n"
      "Ctrl+Z  Undo\nCtrl+Shift+Z  Redo\nCtrl+N  New playlist\nCtrl+O  Open files\nCtrl+S  Save playlist\n"
      "F2  Edit playlist value\nCtrl+F  Focus collection search\nCtrl+Q  Quit\nCtrl+,  Preferences");
  gtk_widget_set_margin_start(label, 24);
  gtk_widget_set_margin_end(label, 24);
  gtk_widget_set_margin_top(label, 24);
  gtk_widget_set_margin_bottom(label, 24);
  adw_dialog_set_child(dialog, label);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
