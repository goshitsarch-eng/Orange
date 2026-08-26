#include "device/deviceproperties.h"

#include "utilities/fileutils.h"

#include <adwaita.h>

void DeviceProperties::Show(GtkWindow *parent, const ConnectedDevice &device) {
  const std::string body = "Name: " + device.friendly_name + "\nBackend: " + device.backend + "\nId: " + device.unique_id +
                           "\nMount: " + device.mount_path + "\nCapacity: " + FileUtils::PrettySize(device.size);
  AdwAlertDialog *dialog = ADW_ALERT_DIALOG(adw_alert_dialog_new("Device properties", body.c_str()));
  adw_alert_dialog_add_response(dialog, "close", "Close");
  adw_dialog_present(ADW_DIALOG(dialog), GTK_WIDGET(parent));
}
