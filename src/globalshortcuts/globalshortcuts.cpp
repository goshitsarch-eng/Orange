#include "globalshortcuts/globalshortcuts.h"

#include "config.h"
#include "core/logging.h"
#include "core/settings.h"

GlobalShortcutsManager::GlobalShortcutsManager() = default;

GlobalShortcutsManager::~GlobalShortcutsManager() {
  if (media_keys_) {
    g_object_unref(media_keys_);
  }
}

void GlobalShortcutsManager::Init() { ReloadSettings(); }

void GlobalShortcutsManager::ReloadSettings() {
  Settings s;
  s.BeginGroup("GlobalShortcuts");
  enabled_ = s.BoolValue("enabled", true);
  if (enabled_) {
    GrabMediaKeys();
  }
}

void GlobalShortcutsManager::GrabMediaKeys() {
  if (media_keys_) {
    return;
  }
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
    return;
  }
  g_dbus_proxy_call(media_keys_, "GrabMediaPlayerKeys", g_variant_new("(su)", "strawberry", 0u), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr,
                    nullptr);
  g_signal_connect(media_keys_, "g-signal", G_CALLBACK(OnMediaKey), this);
}

void GlobalShortcutsManager::OnMediaKey(GDBusProxy *, const char *, const char *signal, GVariant *parameters, gpointer data) {
  auto *self = static_cast<GlobalShortcutsManager *>(data);
  if (!self->enabled_ || g_strcmp0(signal, "MediaPlayerKeyPressed") != 0 || !parameters) {
    return;
  }
  const char *app = nullptr;
  const char *key = nullptr;
  g_variant_get(parameters, "(&s&s)", &app, &key);
  if (!key) {
    return;
  }
  if (g_strcmp0(key, "Play") == 0 || g_strcmp0(key, "Pause") == 0 || g_strcmp0(key, "PlayPause") == 0) {
    self->PlayPause.Emit();
  } else if (g_strcmp0(key, "Stop") == 0) {
    self->Stop.Emit();
  } else if (g_strcmp0(key, "Next") == 0) {
    self->Next.Emit();
  } else if (g_strcmp0(key, "Previous") == 0) {
    self->Previous.Emit();
  } else if (g_strcmp0(key, "VolumeUp") == 0) {
    self->VolumeUp.Emit();
  } else if (g_strcmp0(key, "VolumeDown") == 0) {
    self->VolumeDown.Emit();
  } else if (g_strcmp0(key, "Mute") == 0) {
    self->Mute.Emit();
  }
}
