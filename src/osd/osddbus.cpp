#include "osd/osddbus.h"

#include "osd/osdart.h"

#include <gio/gio.h>

void OSDDbus::ShowMessage(const std::string &summary, const std::string &body, const std::string &icon,
                          const std::vector<unsigned char> &art) {
  GNotification *notification = g_notification_new(summary.c_str());
  g_notification_set_body(notification, body.c_str());
  if (OSDArt::ShouldAttachArt(true, art)) {
    GBytes *bytes = g_bytes_new(art.data(), art.size());
    GIcon *gicon = g_bytes_icon_new(bytes);
    g_notification_set_icon(notification, gicon);
    g_object_unref(gicon);
    g_bytes_unref(bytes);
  } else {
    GIcon *gicon = g_themed_icon_new(icon.c_str());
    g_notification_set_icon(notification, gicon);
    g_object_unref(gicon);
  }
  if (GApplication *app = g_application_get_default()) {
    g_application_send_notification(app, "strawberry-osd", notification);
  }
  g_object_unref(notification);
}
