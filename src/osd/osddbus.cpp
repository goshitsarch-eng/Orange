#include "osd/osddbus.h"

#include <gio/gio.h>

void OSDDbus::ShowMessage(const std::string &summary, const std::string &body, const std::string &icon) {
  GNotification *notification = g_notification_new(summary.c_str());
  g_notification_set_body(notification, body.c_str());
  GIcon *gicon = g_themed_icon_new(icon.c_str());
  g_notification_set_icon(notification, gicon);
  g_object_unref(gicon);
  if (GApplication *app = g_application_get_default()) {
    g_application_send_notification(app, "strawberry-osd", notification);
  }
  g_object_unref(notification);
}
