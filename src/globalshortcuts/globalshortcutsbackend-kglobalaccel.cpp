#include "globalshortcuts/globalshortcutsbackend-kglobalaccel.h"

#include "globalshortcuts/globalshortcuts.h"

GlobalShortcutsBackendKGlobalAccel::GlobalShortcutsBackendKGlobalAccel(GlobalShortcutsManager *manager)
    : GlobalShortcutsBackend(manager, Type::KGlobalAccel) {}

GlobalShortcutsBackendKGlobalAccel::~GlobalShortcutsBackendKGlobalAccel() { DoUnregister(); }

bool GlobalShortcutsBackendKGlobalAccel::IsAvailable() const {
  GDBusConnection *bus = g_bus_get_sync(G_BUS_TYPE_SESSION, nullptr, nullptr);
  if (!bus) {
    return false;
  }
  GError *error = nullptr;
  GDBusProxy *proxy = g_dbus_proxy_new_sync(bus, G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES, nullptr, "org.gnome.SettingsDaemon.MediaKeys",
                                            "/org/gnome/SettingsDaemon/MediaKeys", "org.gnome.SettingsDaemon.MediaKeys", nullptr, &error);
  g_object_unref(bus);
  if (error) {
    g_error_free(error);
  }
  if (!proxy) {
    return false;
  }
  g_object_unref(proxy);
  return true;
}

bool GlobalShortcutsBackendKGlobalAccel::DoRegister() {
  GError *error = nullptr;
  media_keys_ = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.gnome.SettingsDaemon.MediaKeys",
                                              "/org/gnome/SettingsDaemon/MediaKeys", "org.gnome.SettingsDaemon.MediaKeys", nullptr, &error);
  if (!media_keys_) {
    if (error) {
      g_error_free(error);
    }
    error = nullptr;
    media_keys_ = g_dbus_proxy_new_for_bus_sync(G_BUS_TYPE_SESSION, G_DBUS_PROXY_FLAGS_NONE, nullptr, "org.gnome.SettingsDaemon",
                                                "/org/gnome/SettingsDaemon/MediaKeys", "org.gnome.SettingsDaemon.MediaKeys", nullptr, &error);
    if (error) {
      g_error_free(error);
    }
  }
  if (!media_keys_) {
    return false;
  }
  g_dbus_proxy_call(media_keys_, "GrabMediaPlayerKeys", g_variant_new("(su)", "strawberry", 0u), G_DBUS_CALL_FLAGS_NONE, -1, nullptr,
                    nullptr, nullptr);
  g_signal_connect(media_keys_, "g-signal", G_CALLBACK(OnMediaKey), this);
  return true;
}

void GlobalShortcutsBackendKGlobalAccel::DoUnregister() {
  if (media_keys_) {
    g_object_unref(media_keys_);
    media_keys_ = nullptr;
  }
}

void GlobalShortcutsBackendKGlobalAccel::OnMediaKey(GDBusProxy *, const char *, const char *signal, GVariant *parameters, gpointer data) {
  auto *self = static_cast<GlobalShortcutsBackendKGlobalAccel *>(data);
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
    self->manager_->PlayPause.Emit();
  } else if (g_strcmp0(key, "Stop") == 0) {
    self->manager_->Stop.Emit();
  } else if (g_strcmp0(key, "Next") == 0) {
    self->manager_->Next.Emit();
  } else if (g_strcmp0(key, "Previous") == 0) {
    self->manager_->Previous.Emit();
  } else if (g_strcmp0(key, "VolumeUp") == 0) {
    self->manager_->VolumeUp.Emit();
  } else if (g_strcmp0(key, "VolumeDown") == 0) {
    self->manager_->VolumeDown.Emit();
  } else if (g_strcmp0(key, "Mute") == 0) {
    self->manager_->Mute.Emit();
  }
}
