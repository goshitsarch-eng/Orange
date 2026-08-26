#ifndef STRAWBERRY_DEVICEPROPERTIES_H
#define STRAWBERRY_DEVICEPROPERTIES_H

#include "device/connecteddevice.h"

#include <gtk/gtk.h>

class DeviceProperties {
 public:
  static void Show(GtkWindow *parent, const ConnectedDevice &device);
};

#endif
