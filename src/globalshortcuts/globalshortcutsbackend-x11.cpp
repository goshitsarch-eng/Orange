#include "globalshortcuts/globalshortcutsbackend-x11.h"

#include "core/settings.h"
#include "globalshortcuts/globalshortcuts.h"
#include "globalshortcuts/keymapper_x11.h"

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <glib-unix.h>
#endif

GlobalShortcutsBackendX11::GlobalShortcutsBackendX11(GlobalShortcutsManager *manager)
    : GlobalShortcutsBackend(manager, Type::X11) {}

GlobalShortcutsBackendX11::~GlobalShortcutsBackendX11() { DoUnregister(); }

bool GlobalShortcutsBackendX11::IsAvailable() const {
#ifdef HAVE_X11
  Display *display = XOpenDisplay(nullptr);
  if (!display) {
    return false;
  }
  XCloseDisplay(display);
  return true;
#else
  return false;
#endif
}

bool GlobalShortcutsBackendX11::DoRegister() {
#ifdef HAVE_X11
  Display *display = XOpenDisplay(nullptr);
  if (!display) {
    return false;
  }
  x11_display_ = display;
  Settings s;
  s.BeginGroup("GlobalShortcuts");
  std::vector<KeySym> keys = {XF86XK_AudioPlay,        XF86XK_AudioPause,       XF86XK_AudioStop,       XF86XK_AudioNext,
                              XF86XK_AudioPrev,        XF86XK_AudioRaiseVolume, XF86XK_AudioLowerVolume, XF86XK_AudioMute};
  const KeySym play = static_cast<KeySym>(KeyMapperX11::KeysymFromName(s.Value("playpause", "MediaPlay")));
  const KeySym next = static_cast<KeySym>(KeyMapperX11::KeysymFromName(s.Value("next", "MediaNext")));
  const KeySym previous = static_cast<KeySym>(KeyMapperX11::KeysymFromName(s.Value("previous", "MediaPrevious")));
  if (play) keys.push_back(play);
  if (next) keys.push_back(next);
  if (previous) keys.push_back(previous);
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
  return true;
#else
  return false;
#endif
}

void GlobalShortcutsBackendX11::DoUnregister() {
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

#ifdef HAVE_X11
gboolean GlobalShortcutsBackendX11::OnX11Fd(gint, GIOCondition, gpointer data) {
  static_cast<GlobalShortcutsBackendX11 *>(data)->HandleX11Event();
  return G_SOURCE_CONTINUE;
}

void GlobalShortcutsBackendX11::HandleX11Event() {
  auto *display = static_cast<Display *>(x11_display_);
  if (!display || !manager_) {
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
      manager_->PlayPause.Emit();
    } else if (keysym == XF86XK_AudioStop) {
      manager_->Stop.Emit();
    } else if (keysym == XF86XK_AudioNext) {
      manager_->Next.Emit();
    } else if (keysym == XF86XK_AudioPrev) {
      manager_->Previous.Emit();
    } else if (keysym == XF86XK_AudioRaiseVolume) {
      manager_->VolumeUp.Emit();
    } else if (keysym == XF86XK_AudioLowerVolume) {
      manager_->VolumeDown.Emit();
    } else if (keysym == XF86XK_AudioMute) {
      manager_->Mute.Emit();
    }
  }
}
#endif
