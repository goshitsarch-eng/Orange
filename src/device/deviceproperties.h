#ifndef STRAWBERRY_DEVICEPROPERTIES_H
#define STRAWBERRY_DEVICEPROPERTIES_H

#include "device/connecteddevice.h"

#include <gtk/gtk.h>

class Application;

class DeviceProperties {
 public:
  static void Show(GtkWindow *parent, Application *app, const ConnectedDevice &device);
};

#endif
