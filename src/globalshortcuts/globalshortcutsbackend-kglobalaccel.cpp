#include "globalshortcuts/globalshortcutsbackend-kglobalaccel.h"

#include "globalshortcuts/globalshortcut.h"
#include "globalshortcuts/globalshortcuts.h"
#include "globalshortcuts/keymapper_x11.h"

namespace {

constexpr char kKGlobalAccelService[] = "org.kde.kglobalaccel";
constexpr char kKGlobalAccelPath[] = "/kglobalaccel";
constexpr char kKGlobalAccelInterface[] = "org.kde.KGlobalAccel";
constexpr char kComponentInterface[] = "org.kde.kglobalaccel.Component";
constexpr unsigned kSetPresent = 0x2;

bool NameHasOwner(const char *name) {
  GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  if (!bus) {
    return false;
  }
  GError *error = nullptr;
  GVariant *reply = g_dbus_connection_call_sync(bus, "org.freedesktop.DBus", "/org/freedesktop/DBus", "org.freedesktop.DBus",
                                                "NameHasOwner", g_variant_new("(s)", name), G_VARIANT_TYPE("(b)"),
                                                G_DBUS_CALL_FLAGS_NONE, 2000, nullptr, &error);
  g_object_unref(bus);
  if (error) {
    g_error_free(error);
    return false;
  }
  if (!reply) {
    return false;
  }
  gboolean owner = FALSE;
  g_variant_get(reply, "(b)", &owner);
  g_variant_unref(reply);
  return owner;
}

GVariant *ActionId(const std::string &id, const std::string &description) {
  GVariantBuilder builder;
  g_variant_builder_init(&builder, G_VARIANT_TYPE("as"));
  g_variant_builder_add(&builder, "s", "strawberry");
  g_variant_builder_add(&builder, "s", id.c_str());
  g_variant_builder_add(&builder, "s", "strawberry");
  g_variant_builder_add(&builder, "s", description.empty() ? id.c_str() : description.c_str());
  return g_variant_builder_end(&builder);
}

}  // namespace

GlobalShortcutsBackendKGlobalAccel::GlobalShortcutsBackendKGlobalAccel(GlobalShortcutsManager *manager)
    : GlobalShortcutsBackend(manager, Type::KGlobalAccel) {}

GlobalShortcutsBackendKGlobalAccel::~GlobalShortcutsBackendKGlobalAccel() { DoUnregister(); }

bool GlobalShortcutsBackendKGlobalAccel::IsKGlobalAccelAvailable() { return NameHasOwner(kKGlobalAccelService); }

bool GlobalShortcutsBackendKGlobalAccel::IsAvailable() const { return IsKGlobalAccelAvailable(); }

bool GlobalShortcutsBackendKGlobalAccel::RegisterShortcut(const GlobalShortcut *shortcut) {
  if (!interface_ || !shortcut || shortcut->id().empty() || shortcut->key().empty()) {
    return false;
  }
  const unsigned qt_key = KeyMapperX11::QtShortcutToKey(shortcut->key());
  if (!qt_key) {
    return false;
  }

  GError *error = nullptr;
  GVariant *register_reply =
      g_dbus_proxy_call_sync(interface_, "doRegister", g_variant_new("(@as)", ActionId(shortcut->id(), shortcut->description())),
                             G_DBUS_CALL_FLAGS_NONE, 3000, nullptr, &error);
  if (error) {
    g_error_free(error);
    error = nullptr;
  }
  if (register_reply) {
    g_variant_unref(register_reply);
  }

  GVariantBuilder keys;
  g_variant_builder_init(&keys, G_VARIANT_TYPE("ai"));
  g_variant_builder_add(&keys, "i", static_cast<gint32>(qt_key));
  GVariant *set_reply =
      g_dbus_proxy_call_sync(interface_, "setShortcut",
                             g_variant_new("(@asaiu)", ActionId(shortcut->id(), shortcut->description()), &keys, kSetPresent),
                             G_DBUS_CALL_FLAGS_NONE, 3000, nullptr, &error);
  if (error) {
    g_error_free(error);
    return false;
  }
  if (set_reply) {
    g_variant_unref(set_reply);
  }
  registered_ids_.push_back(shortcut->id());
  return true;
}

void GlobalShortcutsBackendKGlobalAccel::SubscribeComponent() {
  if (!interface_) {
    return;
  }
  GError *error = nullptr;
  GVariant *reply = g_dbus_proxy_call_sync(interface_, "getComponent", g_variant_new("(s)", "strawberry"), G_DBUS_CALL_FLAGS_NONE,
                                           3000, nullptr, &error);
  if (error) {
    g_error_free(error);
    return;
  }
  if (!reply) {
    return;
  }
  const char *path = nullptr;
  g_variant_get(reply, "(&o)", &path);
  GDBusConnection *bus = g_dbus_proxy_get_connection(interface_);
  if (bus && path) {
    signal_id_ = g_dbus_connection_signal_subscribe(bus, kKGlobalAccelService, kComponentInterface, "globalShortcutPressed", path,
                                                    nullptr, G_DBUS_SIGNAL_FLAGS_NONE, OnShortcutPressed, this, nullptr);
  }
  g_variant_unref(reply);
}

bool GlobalShortcutsBackendKGlobalAccel::DoRegister() {
  if (!IsKGlobalAccelAvailable() || !manager_) {
    return false;
  }
  GError *error = nullptr;
  interface_ = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, nullptr, kKGlobalAccelService,
                                             kKGlobalAccelPath, kKGlobalAccelInterface, nullptr, &error);
  if (error) {
    g_error_free(error);
  }
  if (!interface_) {
    return false;
  }
  registered_ids_.clear();
  for (const auto &shortcut : manager_->shortcuts()) {
    RegisterShortcut(shortcut.get());
  }
  SubscribeComponent();
  return interface_ != nullptr;
}

void GlobalShortcutsBackendKGlobalAccel::DoUnregister() {
  if (interface_ && signal_id_) {
    g_dbus_connection_signal_unsubscribe(g_dbus_proxy_get_connection(interface_), signal_id_);
    signal_id_ = 0;
  }
  if (interface_) {
    for (const std::string &id : registered_ids_) {
      GError *error = nullptr;
      GVariant *reply = g_dbus_proxy_call_sync(interface_, "unRegister", g_variant_new("(@as)", ActionId(id, id)),
                                               G_DBUS_CALL_FLAGS_NONE, 2000, nullptr, &error);
      if (error) {
        g_error_free(error);
      }
      if (reply) {
        g_variant_unref(reply);
      }
    }
    registered_ids_.clear();
    g_object_unref(interface_);
    interface_ = nullptr;
  }
}

void GlobalShortcutsBackendKGlobalAccel::OnShortcutPressed(GDBusConnection *, const gchar *, const gchar *, const gchar *,
                                                           const gchar *, GVariant *parameters, gpointer data) {
  auto *self = static_cast<GlobalShortcutsBackendKGlobalAccel *>(data);
  if (!self->manager_ || !parameters) {
    return;
  }
  const char *component = nullptr;
  const char *action = nullptr;
  gint64 timestamp = 0;
  g_variant_get(parameters, "(&s&sx)", &component, &action, &timestamp);
  if (g_strcmp0(component, "strawberry") != 0 || !action) {
    return;
  }
  self->manager_->Emit(action);
}

GlobalShortcutsBackendGnome::GlobalShortcutsBackendGnome(GlobalShortcutsManager *manager)
    : GlobalShortcutsBackend(manager, Type::Gnome) {}

GlobalShortcutsBackendGnome::~GlobalShortcutsBackendGnome() { DoUnregister(); }

bool GlobalShortcutsBackendGnome::IsAvailable() const {
  return NameHasOwner("org.gnome.SettingsDaemon.MediaKeys") || NameHasOwner("org.gnome.SettingsDaemon");
}

bool GlobalShortcutsBackendGnome::DoRegister() {
  GError *error = nullptr;
  media_keys_ = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, nullptr,
                                              "org.gnome.SettingsDaemon.MediaKeys", "/org/gnome/SettingsDaemon/MediaKeys",
                                              "org.gnome.SettingsDaemon.MediaKeys", nullptr, &error);
  if (!media_keys_) {
    if (error) {
      g_error_free(error);
    }
    error = nullptr;
    media_keys_ = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.gnome.SettingsDaemon",
                                                "/org/gnome/SettingsDaemon/MediaKeys", "org.gnome.SettingsDaemon.MediaKeys",
                                                nullptr, &error);
    if (error) {
      g_error_free(error);
    }
  }
  if (!media_keys_) {
    return false;
  }
  g_dbus_proxy_call(media_keys_, "GrabMediaPlayerKeys", g_variant_new("(su)", "strawberry", 0u), G_DBUS_CALL_FLAGS_NONE, -1,
                    nullptr, nullptr, nullptr);
  g_signal_connect(media_keys_, "g-signal", G_CALLBACK(OnMediaKey), this);
  return true;
}

void GlobalShortcutsBackendGnome::DoUnregister() {
  if (media_keys_) {
    g_dbus_proxy_call(media_keys_, "ReleaseMediaPlayerKeys", g_variant_new("(s)", "strawberry"), G_DBUS_CALL_FLAGS_NONE, -1,
                      nullptr, nullptr, nullptr);
    g_object_unref(media_keys_);
    media_keys_ = nullptr;
  }
}

void GlobalShortcutsBackendGnome::OnMediaKey(GDBusProxy *, const char *, const char *signal, GVariant *parameters, gpointer data) {
  auto *self = static_cast<GlobalShortcutsBackendGnome *>(data);
  if (!self->manager_ || g_strcmp0(signal, "MediaPlayerKeyPressed") != 0 || !parameters) {
    return;
  }
  const char *app = nullptr;
  const char *key = nullptr;
  g_variant_get(parameters, "(&s&s)", &app, &key);
  if (!key) {
    return;
  }
  if (g_strcmp0(key, "Play") == 0 || g_strcmp0(key, "Pause") == 0 || g_strcmp0(key, "PlayPause") == 0) {
    self->manager_->Emit("play_pause");
  } else if (g_strcmp0(key, "Stop") == 0) {
    self->manager_->Emit("stop");
  } else if (g_strcmp0(key, "Next") == 0) {
    self->manager_->Emit("next_track");
  } else if (g_strcmp0(key, "Previous") == 0) {
    self->manager_->Emit("prev_track");
  } else if (g_strcmp0(key, "VolumeUp") == 0) {
    self->manager_->Emit("inc_volume");
  } else if (g_strcmp0(key, "VolumeDown") == 0) {
    self->manager_->Emit("dec_volume");
  } else if (g_strcmp0(key, "Mute") == 0) {
    self->manager_->Emit("mute");
  } else if (g_strcmp0(key, "FastForward") == 0) {
    self->manager_->Emit("seek_forward");
  } else if (g_strcmp0(key, "Rewind") == 0) {
    self->manager_->Emit("seek_backward");
  }
}
