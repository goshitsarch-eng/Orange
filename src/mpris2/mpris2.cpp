#include "mpris2/mpris2.h"
#include "core/application.h"
#include "core/logging.h"
#include "core/player.h"
#include "config.h"

#ifdef HAVE_MPRIS2
static const gchar *kMprisXml =
"<node>"
"  <interface name='org.mpris.MediaPlayer2'>"
"    <method name='Raise'/>"
"    <method name='Quit'/>"
"    <property name='CanQuit' type='b' access='read'/>"
"    <property name='CanRaise' type='b' access='read'/>"
"    <property name='HasTrackList' type='b' access='read'/>"
"    <property name='Identity' type='s' access='read'/>"
"    <property name='DesktopEntry' type='s' access='read'/>"
"    <property name='SupportedUriSchemes' type='as' access='read'/>"
"    <property name='SupportedMimeTypes' type='as' access='read'/>"
"  </interface>"
"  <interface name='org.mpris.MediaPlayer2.Player'>"
"    <method name='Next'/>"
"    <method name='Previous'/>"
"    <method name='Pause'/>"
"    <method name='PlayPause'/>"
"    <method name='Stop'/>"
"    <method name='Play'/>"
"    <method name='Seek'><arg type='x' name='Offset'/></method>"
"    <method name='SetPosition'><arg type='o' name='TrackId'/><arg type='x' name='Position'/></method>"
"    <method name='OpenUri'><arg type='s' name='Uri'/></method>"
"    <property name='PlaybackStatus' type='s' access='read'/>"
"    <property name='Rate' type='d' access='readwrite'/>"
"    <property name='Metadata' type='a{sv}' access='read'/>"
"    <property name='Volume' type='d' access='readwrite'/>"
"    <property name='Position' type='x' access='read'/>"
"    <property name='CanGoNext' type='b' access='read'/>"
"    <property name='CanGoPrevious' type='b' access='read'/>"
"    <property name='CanPlay' type='b' access='read'/>"
"    <property name='CanPause' type='b' access='read'/>"
"    <property name='CanSeek' type='b' access='read'/>"
"    <property name='CanControl' type='b' access='read'/>"
"  </interface>"
"</node>";

static void HandleMethod(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar *method, GVariant *parameters, GDBusMethodInvocation *invocation, gpointer user_data) {
  auto *self = static_cast<Mpris2*>(user_data);
  Application *app = self ? self->app() : nullptr;
  if (app && app->player()) {
    if (g_strcmp0(method, "Play") == 0) app->player()->Play();
    else if (g_strcmp0(method, "Pause") == 0) app->player()->Pause();
    else if (g_strcmp0(method, "PlayPause") == 0) app->player()->PlayPause();
    else if (g_strcmp0(method, "Stop") == 0) app->player()->Stop();
    else if (g_strcmp0(method, "Next") == 0) app->player()->Next();
    else if (g_strcmp0(method, "Previous") == 0) app->player()->Previous();
    else if (g_strcmp0(method, "Seek") == 0) {
      gint64 offset = 0;
      g_variant_get(parameters, "(x)", &offset);
      app->player()->SeekTo(app->player()->engine()->position_nanosec() / 1000000000LL + offset / 1000000);
    }
  }
  g_dbus_method_invocation_return_value(invocation, nullptr);
}

static GVariant *HandleGet(GDBusConnection*, const gchar*, const gchar*, const gchar *interface, const gchar *property, GError**, gpointer user_data) {
  auto *self = static_cast<Mpris2*>(user_data);
  Application *app = self ? self->app() : nullptr;
  if (g_strcmp0(property, "Identity") == 0) return g_variant_new_string("Strawberry");
  if (g_strcmp0(property, "DesktopEntry") == 0) return g_variant_new_string("org.strawberrymusicplayer.strawberry");
  if (g_strcmp0(property, "CanQuit") == 0 || g_strcmp0(property, "CanRaise") == 0 || g_strcmp0(property, "CanPlay") == 0 ||
      g_strcmp0(property, "CanPause") == 0 || g_strcmp0(property, "CanGoNext") == 0 || g_strcmp0(property, "CanGoPrevious") == 0 ||
      g_strcmp0(property, "CanSeek") == 0 || g_strcmp0(property, "CanControl") == 0) return g_variant_new_boolean(TRUE);
  if (g_strcmp0(property, "HasTrackList") == 0) return g_variant_new_boolean(FALSE);
  if (g_strcmp0(property, "PlaybackStatus") == 0) {
    if (!app || !app->player()) return g_variant_new_string("Stopped");
    switch (app->player()->GetState()) {
      case GstEngine::State::Playing: return g_variant_new_string("Playing");
      case GstEngine::State::Paused: return g_variant_new_string("Paused");
      default: return g_variant_new_string("Stopped");
    }
  }
  if (g_strcmp0(property, "Rate") == 0) return g_variant_new_double(1.0);
  if (g_strcmp0(property, "Volume") == 0) return g_variant_new_double(app && app->player() ? app->player()->GetVolume() / 100.0 : 1.0);
  if (g_strcmp0(property, "Position") == 0) {
    const int64_t pos = app && app->player() ? app->player()->engine()->position_nanosec() / 1000 : 0;
    return g_variant_new_int64(pos);
  }
  if (g_strcmp0(property, "SupportedUriSchemes") == 0 || g_strcmp0(property, "SupportedMimeTypes") == 0) {
    GVariantBuilder b; g_variant_builder_init(&b, G_VARIANT_TYPE("as"));
    if (g_strcmp0(property, "SupportedUriSchemes") == 0) g_variant_builder_add(&b, "s", "file");
    else g_variant_builder_add(&b, "s", "audio/mpeg");
    return g_variant_builder_end(&b);
  }
  if (g_strcmp0(property, "Metadata") == 0) {
    GVariantBuilder b; g_variant_builder_init(&b, G_VARIANT_TYPE("a{sv}"));
    if (app && app->player()) {
      const Song song = app->player()->current_song();
      g_variant_builder_add(&b, "{sv}", "xesam:title", g_variant_new_string(song.title().c_str()));
      g_variant_builder_add(&b, "{sv}", "xesam:album", g_variant_new_string(song.album().c_str()));
      g_variant_builder_add(&b, "{sv}", "xesam:artist", g_variant_new_strv((const gchar *[]){song.artist().c_str(), nullptr}, -1));
      g_variant_builder_add(&b, "{sv}", "xesam:url", g_variant_new_string(song.url().c_str()));
    }
    return g_variant_builder_end(&b);
  }
  (void)interface;
  return nullptr;
}

static gboolean HandleSet(GDBusConnection*, const gchar*, const gchar*, const gchar*, const gchar*, GVariant*, GError**, gpointer) { return TRUE; }

static const GDBusInterfaceVTable kVtable = { HandleMethod, HandleGet, HandleSet, {nullptr} };
#endif

Mpris2::Mpris2(Application *app) : app_(app) {
#ifdef HAVE_MPRIS2
  owner_id_ = g_bus_own_name(G_BUS_TYPE_SESSION, "org.mpris.MediaPlayer2.strawberry", G_BUS_NAME_OWNER_FLAGS_NONE, OnBusAcquired, nullptr, nullptr, this, nullptr);
#endif
}
Mpris2::~Mpris2() {
#ifdef HAVE_MPRIS2
  if (owner_id_) g_bus_unown_name(owner_id_);
#endif
}
void Mpris2::OnBusAcquired(GDBusConnection *connection, const gchar*, gpointer user_data) {
#ifdef HAVE_MPRIS2
  auto *self = static_cast<Mpris2*>(user_data);
  self->connection_ = connection;
  GError *error = nullptr;
  GDBusNodeInfo *info = g_dbus_node_info_new_for_xml(kMprisXml, &error);
  if (!info) { if (error) g_error_free(error); return; }
  g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2", info->interfaces[0], &kVtable, self, nullptr, nullptr);
  g_dbus_connection_register_object(connection, "/org/mpris/MediaPlayer2", info->interfaces[1], &kVtable, self, nullptr, nullptr);
  g_dbus_node_info_unref(info);
#else
  (void)connection; (void)user_data;
#endif
}
void Mpris2::EmitSeeked(int64_t) {}
void Mpris2::EmitPlaybackStatus() {}
