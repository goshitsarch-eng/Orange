#include "globalshortcuts/globalshortcutsbackend-portal.h"

#include "globalshortcuts/globalshortcuts.h"
#include "globalshortcuts/globalshortcutsportal.h"

#include <string>

namespace {

constexpr char kPortalName[] = "org.freedesktop.portal.Desktop";
constexpr char kPortalPath[] = "/org/freedesktop/portal/desktop";
constexpr char kPortalInterface[] = "org.freedesktop.portal.GlobalShortcuts";

}  // namespace

GlobalShortcutsBackendPortal::GlobalShortcutsBackendPortal(GlobalShortcutsManager *manager)
    : GlobalShortcutsBackend(manager, Type::Portal) {}

GlobalShortcutsBackendPortal::~GlobalShortcutsBackendPortal() { DoUnregister(); }

bool GlobalShortcutsBackendPortal::IsPortalAvailable() {
  GError *error = nullptr;
  GDBusConnection *connection = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
  if (!connection) {
    if (error) {
      g_error_free(error);
    }
    return false;
  }
  GVariant *reply = g_dbus_connection_call_sync(connection, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
                                                "GetNameOwner", g_variant_new("(s)", kPortalName), G_VARIANT_TYPE("(s)"),
                                                G_DBUS_CALL_FLAGS_NONE, 500, nullptr, &error);
  if (error) {
    g_error_free(error);
  }
  if (reply) {
    g_variant_unref(reply);
  }
  g_object_unref(connection);
  return reply != nullptr;
}

bool GlobalShortcutsBackendPortal::IsAvailable() const { return IsPortalAvailable(); }

bool GlobalShortcutsBackendPortal::DoRegister() {
  GError *error = nullptr;
  connection_ = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, &error);
  if (!connection_) {
    if (error) {
      g_error_free(error);
    }
    return false;
  }
  activated_id_ = g_dbus_connection_signal_subscribe(connection_, kPortalName, kPortalInterface, "Activated", nullptr, nullptr,
                                                     G_DBUS_SIGNAL_FLAGS_NONE, OnActivated, manager_, nullptr);

  GVariantBuilder options;
  g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
  g_variant_builder_add(&options, "{sv}", "handle_token", g_variant_new_string("strawberry"));
  g_variant_builder_add(&options, "{sv}", "session_handle_token", g_variant_new_string("strawberry"));
  GVariant *reply = g_dbus_connection_call_sync(connection_, kPortalName, kPortalPath, kPortalInterface, "CreateSession",
                                                g_variant_new("(a{sv})", &options), G_VARIANT_TYPE("(o)"), G_DBUS_CALL_FLAGS_NONE, 2000,
                                                nullptr, &error);
  if (!reply) {
    if (error) {
      g_error_free(error);
    }
    return true;
  }
  const gchar *request = nullptr;
  g_variant_get(reply, "(&o)", &request);
  if (request) {
    response_id_ = g_dbus_connection_signal_subscribe(
        connection_, kPortalName, "org.freedesktop.portal.Request", "Response", request, nullptr, G_DBUS_SIGNAL_FLAGS_NONE,
        +[](GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *, GVariant *parameters, gpointer data) {
          auto *self = static_cast<GlobalShortcutsBackendPortal *>(data);
          if (!self || !parameters) {
            return;
          }
          guint code = 1;
          GVariant *results = nullptr;
          g_variant_get(parameters, "(u@a{sv})", &code, &results);
          if (code == 0 && results) {
            const gchar *session = nullptr;
            if (g_variant_lookup(results, "session_handle", "&s", &session) && session) {
              self->BindSession(session);
            } else if (g_variant_lookup(results, "session_handle", "&o", &session) && session) {
              self->BindSession(session);
            }
          }
          if (results) {
            g_variant_unref(results);
          }
        },
        this, nullptr);
  }
  g_variant_unref(reply);
  return true;
}

void GlobalShortcutsBackendPortal::BindSession(const std::string &session_path) {
  if (!connection_ || !manager_ || session_path.empty()) {
    return;
  }
  session_path_ = session_path;
  GVariantBuilder shortcuts;
  g_variant_builder_init(&shortcuts, G_VARIANT_TYPE("a(sa{sv})"));
  for (const auto &binding : GlobalShortcutsPortal::Bindings(*manager_)) {
    GVariantBuilder props;
    g_variant_builder_init(&props, G_VARIANT_TYPE("a{sv}"));
    g_variant_builder_add(&props, "{sv}", "description", g_variant_new_string(manager_->FriendlyName(binding.first).c_str()));
    g_variant_builder_add(&props, "{sv}", "preferred_trigger", g_variant_new_string(binding.second.c_str()));
    g_variant_builder_add(&shortcuts, "(sa{sv})", binding.first.c_str(), &props);
  }
  GVariantBuilder options;
  g_variant_builder_init(&options, G_VARIANT_TYPE("a{sv}"));
  g_dbus_connection_call(connection_, kPortalName, kPortalPath, kPortalInterface, "BindShortcuts",
                         g_variant_new("(oa(sa{sv})sa{sv})", session_path.c_str(), &shortcuts, "", &options), nullptr,
                         G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr, nullptr);
}

void GlobalShortcutsBackendPortal::DoUnregister() {
  if (connection_ && response_id_) {
    g_dbus_connection_signal_unsubscribe(connection_, response_id_);
    response_id_ = 0;
  }
  if (connection_ && activated_id_) {
    g_dbus_connection_signal_unsubscribe(connection_, activated_id_);
    activated_id_ = 0;
  }
  if (connection_) {
    g_object_unref(connection_);
    connection_ = nullptr;
  }
  session_path_.clear();
}

void GlobalShortcutsBackendPortal::OnActivated(GDBusConnection *, const gchar *, const gchar *, const gchar *, const gchar *,
                                               GVariant *parameters, gpointer data) {
  auto *manager = static_cast<GlobalShortcutsManager *>(data);
  if (!manager || !parameters) {
    return;
  }
  const gchar *session = nullptr;
  const gchar *shortcut_id = nullptr;
  guint64 timestamp = 0;
  GVariant *options = nullptr;
  g_variant_get(parameters, "(&o&st@a{sv})", &session, &shortcut_id, &timestamp, &options);
  if (shortcut_id) {
    manager->Emit(shortcut_id);
  }
  if (options) {
    g_variant_unref(options);
  }
  (void)session;
}
