#include "globalshortcuts/globalshortcuts.h"

#include "core/logging.h"
#include "core/settings.h"

#ifdef HAVE_X11
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#include <glib-unix.h>
#endif

GlobalShortcutsManager::GlobalShortcutsManager() = default;

GlobalShortcutsManager::~GlobalShortcutsManager() { UngrabAll(); }

void GlobalShortcutsManager::Init() { ReloadSettings(); }

void GlobalShortcutsManager::UngrabAll() {
  if (media_keys_) {
    g_object_unref(media_keys_);
    media_keys_ = nullptr;
  }
#ifdef HAVE_X11
  if (x11_watch_id_) {
    g_source_remove(x11_watch_id_);
    x11_watch_id_ = 0;
  }
  if (x11_display_) {
    auto *display = static_cast<Display *>(x11_display_);
    const Window root = DefaultRootWindow(display);
    for (unsigned int code : x11_codes_) {
      XUngrabKey(display, static_cast<KeyCode>(code), AnyModifier, root);
    }
    XCloseDisplay(display);
    x11_display_ = nullptr;
    x11_codes_.clear();
  }
#endif
}

void GlobalShortcutsManager::ReloadSettings() {
  UngrabAll();
  Settings s;
  s.BeginGroup("GlobalShortcuts");
  enabled_ = s.BoolValue("enabled", true);
  if (!enabled_) {
    return;
  }
  const bool gnome = GrabMediaKeys();
  GrabX11Keys(!gnome);
}

bool GlobalShortcutsManager::GrabMediaKeys() {
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
  g_dbus_proxy_call(media_keys_, "GrabMediaPlayerKeys", g_variant_new("(su)", "strawberry", 0u), G_DBUS_CALL_FLAGS_NONE, -1, nullptr, nullptr,
                    nullptr);
  g_signal_connect(media_keys_, "g-signal", G_CALLBACK(OnMediaKey), this);
  return true;
}

#ifdef HAVE_X11
unsigned long GlobalShortcutsManager::KeysymFromName(const std::string &name) const {
  if (name.empty() || name == "MediaPlay" || name == "XF86AudioPlay" || name == "Play") {
    return XF86XK_AudioPlay;
  }
  if (name == "MediaPause" || name == "XF86AudioPause" || name == "Pause") {
    return XF86XK_AudioPause;
  }
  if (name == "MediaStop" || name == "XF86AudioStop" || name == "Stop") {
    return XF86XK_AudioStop;
  }
  if (name == "MediaNext" || name == "XF86AudioNext" || name == "Next") {
    return XF86XK_AudioNext;
  }
  if (name == "MediaPrevious" || name == "XF86AudioPrev" || name == "Previous") {
    return XF86XK_AudioPrev;
  }
  if (name == "XF86AudioRaiseVolume" || name == "VolumeUp") {
    return XF86XK_AudioRaiseVolume;
  }
  if (name == "XF86AudioLowerVolume" || name == "VolumeDown") {
    return XF86XK_AudioLowerVolume;
  }
  if (name == "XF86AudioMute" || name == "Mute") {
    return XF86XK_AudioMute;
  }
  return XStringToKeysym(name.c_str());
}

void GlobalShortcutsManager::GrabX11Keys(bool include_media) {
  Display *display = XOpenDisplay(nullptr);
  if (!display) {
    return;
  }
  x11_display_ = display;
  Settings s;
  s.BeginGroup("GlobalShortcuts");
  std::vector<KeySym> keys;
  if (include_media) {
    keys.push_back(XF86XK_AudioPlay);
    keys.push_back(XF86XK_AudioPause);
    keys.push_back(XF86XK_AudioStop);
    keys.push_back(XF86XK_AudioNext);
    keys.push_back(XF86XK_AudioPrev);
    keys.push_back(XF86XK_AudioRaiseVolume);
    keys.push_back(XF86XK_AudioLowerVolume);
    keys.push_back(XF86XK_AudioMute);
  }
  const KeySym play = static_cast<KeySym>(KeysymFromName(s.Value("playpause", "MediaPlay")));
  const KeySym next = static_cast<KeySym>(KeysymFromName(s.Value("next", "MediaNext")));
  const KeySym previous = static_cast<KeySym>(KeysymFromName(s.Value("previous", "MediaPrevious")));
  if (play && (!include_media || play != XF86XK_AudioPlay)) {
    keys.push_back(play);
  }
  if (next && (!include_media || next != XF86XK_AudioNext)) {
    keys.push_back(next);
  }
  if (previous && (!include_media || previous != XF86XK_AudioPrev)) {
    keys.push_back(previous);
  }
  const Window root = DefaultRootWindow(display);
  for (KeySym keysym : keys) {
    const KeyCode code = XKeysymToKeycode(display, keysym);
    if (!code) {
      continue;
    }
    XGrabKey(display, code, AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
    x11_codes_.push_back(code);
  }
  XSync(display, False);
  x11_watch_id_ = g_unix_fd_add(ConnectionNumber(display), G_IO_IN, OnX11Fd, this);
}

gboolean GlobalShortcutsManager::OnX11Fd(gint, GIOCondition, gpointer data) {
  static_cast<GlobalShortcutsManager *>(data)->HandleX11Event();
  return G_SOURCE_CONTINUE;
}

void GlobalShortcutsManager::HandleX11Event() {
  auto *display = static_cast<Display *>(x11_display_);
  if (!display || !enabled_) {
    return;
  }
  while (XPending(display)) {
    XEvent event;
    XNextEvent(display, &event);
    if (event.type != KeyPress) {
      continue;
    }
    const KeySym keysym = XLookupKeysym(&event.xkey, 0);
    if (keysym == XF86XK_AudioPlay || keysym == XF86XK_AudioPause) {
      PlayPause.Emit();
    } else if (keysym == XF86XK_AudioStop) {
      Stop.Emit();
    } else if (keysym == XF86XK_AudioNext) {
      Next.Emit();
    } else if (keysym == XF86XK_AudioPrev) {
      Previous.Emit();
    } else if (keysym == XF86XK_AudioRaiseVolume) {
      VolumeUp.Emit();
    } else if (keysym == XF86XK_AudioLowerVolume) {
      VolumeDown.Emit();
    } else if (keysym == XF86XK_AudioMute) {
      Mute.Emit();
    } else {
      Settings s;
      s.BeginGroup("GlobalShortcuts");
      if (keysym == KeysymFromName(s.Value("playpause", "MediaPlay"))) {
        PlayPause.Emit();
      } else if (keysym == KeysymFromName(s.Value("next", "MediaNext"))) {
        Next.Emit();
      } else if (keysym == KeysymFromName(s.Value("previous", "MediaPrevious"))) {
        Previous.Emit();
      }
    }
  }
}
#else
void GlobalShortcutsManager::GrabX11Keys(bool) {}
#endif

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
