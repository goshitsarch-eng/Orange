#include "osd/osd.h"
#include "core/settings.h"
#include <gio/gio.h>
void OSD::ReloadSettings() {
  Settings s; s.BeginGroup("OSD");
  enabled_ = s.BoolValue("enabled", true);
  show_art_ = s.BoolValue("showart", true);
}
void OSD::ShowMessage(const std::string &summary, const std::string &body, const std::string &icon) {
  if (!enabled_) return;
  GNotification *notification = g_notification_new(summary.c_str());
  g_notification_set_body(notification, body.c_str());
  GIcon *gicon = g_themed_icon_new(icon.c_str());
  g_notification_set_icon(notification, gicon);
  g_object_unref(gicon);
  g_application_send_notification(g_application_get_default(), "strawberry-osd", notification);
  g_object_unref(notification);
}
void OSD::SongChanged(const Song &song) {
  ShowMessage(song.PrettyTitle(), song.EffectiveAlbumartist() + (song.album().empty() ? "" : "\n" + song.album()));
}
