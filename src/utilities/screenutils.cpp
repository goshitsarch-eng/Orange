#include "utilities/screenutils.h"

#include <gdk/gdk.h>

namespace ScreenUtils {

int Width() {
  GdkDisplay *display = gdk_display_get_default();
  if (!display) {
    return 0;
  }
  GdkMonitor *monitor = GDK_IS_DISPLAY(display) ? gdk_display_get_monitor_at_surface(display, nullptr) : nullptr;
  if (!monitor) {
    GListModel *monitors = gdk_display_get_monitors(display);
    if (monitors && g_list_model_get_n_items(monitors) > 0) {
      monitor = GDK_MONITOR(g_list_model_get_item(monitors, 0));
    }
  }
  if (!monitor) {
    return 0;
  }
  GdkRectangle geo;
  gdk_monitor_get_geometry(monitor, &geo);
  return geo.width;
}

int Height() {
  GdkDisplay *display = gdk_display_get_default();
  if (!display) {
    return 0;
  }
  GListModel *monitors = gdk_display_get_monitors(display);
  if (!monitors || g_list_model_get_n_items(monitors) == 0) {
    return 0;
  }
  auto *monitor = GDK_MONITOR(g_list_model_get_item(monitors, 0));
  GdkRectangle geo;
  gdk_monitor_get_geometry(monitor, &geo);
  g_object_unref(monitor);
  return geo.height;
}

}  // namespace ScreenUtils
