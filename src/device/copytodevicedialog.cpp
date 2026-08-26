#include "device/copytodevicedialog.h"

#include "core/application.h"
#include "widgets/freespacebar.h"

#include <adwaita.h>

void CopyToDeviceDialog::Show(GtkWindow *parent, Application *app) {
  AdwDialog *dialog = adw_dialog_new();
  adw_dialog_set_title(dialog, "Copy to device");
  GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 8);
  gtk_widget_set_margin_start(box, 16);
  gtk_widget_set_margin_end(box, 16);
  gtk_widget_set_margin_top(box, 16);
  gtk_widget_set_margin_bottom(box, 16);
  app->device_manager()->Rescan();
  GtkWidget *list = gtk_list_box_new();
  gtk_widget_add_css_class(list, "boxed-list");
  gtk_box_append(GTK_BOX(box), list);
  for (const ConnectedDevice &device : app->device_manager()->devices()) {
    GtkWidget *row = adw_action_row_new();
    adw_preferences_row_set_title(ADW_PREFERENCES_ROW(row), device.friendly_name.c_str());
    adw_action_row_set_subtitle(ADW_ACTION_ROW(row), device.mount_path.empty() ? device.backend.c_str() : device.mount_path.c_str());
    GtkWidget *copy = gtk_button_new_with_label("Copy playlist");
    g_object_set_data_full(G_OBJECT(copy), "device-id", g_strdup(device.unique_id.c_str()), g_free);
    g_signal_connect(copy, "clicked", G_CALLBACK(+[](GtkButton *btn, gpointer data) {
                       auto *application = static_cast<Application *>(data);
                       const char *id = static_cast<const char *>(g_object_get_data(G_OBJECT(btn), "device-id"));
                       if (id && application->playlist_manager()->active()) {
                         const bool ok = application->device_manager()->CopySongs(id, application->playlist_manager()->active()->songs());
                         gtk_button_set_label(btn, ok ? "Copied" : "Failed");
                       }
                     }),
                     app);
    adw_action_row_add_suffix(ADW_ACTION_ROW(row), copy);
    gtk_list_box_append(GTK_LIST_BOX(list), row);
    if (!device.mount_path.empty()) {
      auto *space = new FreeSpaceBar();
      space->SetPath(device.mount_path);
      gtk_box_append(GTK_BOX(box), space->widget());
      g_signal_connect(space->widget(), "destroy", G_CALLBACK(+[](GtkWidget *, gpointer data) {
                         delete static_cast<FreeSpaceBar *>(data);
                       }),
                       space);
    }
  }
  adw_dialog_set_child(dialog, box);
  adw_dialog_present(dialog, GTK_WIDGET(parent));
}
