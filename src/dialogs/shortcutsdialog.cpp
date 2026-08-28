#include "dialogs/shortcutsdialog.h"
#include "dialogs/dialogchrome.h"

#include "dialogs/shortcutscatalog.h"
#include "translations/translations.h"

#include <adwaita.h>

void ShortcutsDialog::Show(GtkWindow *parent) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, Translations::CStr("Keyboard shortcuts"));
  adw_dialog_set_content_width(dialog, 420);
  adw_dialog_set_content_height(dialog, 480);
  GtkWidget *scroll = gtk_scrolled_window_new();
  GtkWidget *label = gtk_label_new(ShortcutsCatalog::Text().c_str());
  gtk_label_set_selectable(GTK_LABEL(label), TRUE);
  gtk_label_set_xalign(GTK_LABEL(label), 0);
  gtk_widget_set_margin_start(label, 24);
  gtk_widget_set_margin_end(label, 24);
  gtk_widget_set_margin_top(label, 24);
  gtk_widget_set_margin_bottom(label, 24);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroll), label);
  DialogChrome::SetContent(dialog, scroll);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
