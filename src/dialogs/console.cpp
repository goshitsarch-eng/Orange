#include "dialogs/console.h"

#include <adwaita.h>

void Console::Show(GtkWindow *parent) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Debug console");
  GtkWidget *view = gtk_text_view_new();
  gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
  gtk_text_buffer_set_text(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)), "Logging is written to the GLib log domain \"strawberry\".", -1);
  adw_dialog_set_child(dialog, view);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
