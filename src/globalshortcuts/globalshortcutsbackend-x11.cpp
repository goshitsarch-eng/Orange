#include "globalshortcuts/globalshortcutsbackend-x11.h"

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

#ifdef HAVE_X11
void GlobalShortcutsBackendX11::GrabKey(unsigned int code, unsigned int modifiers, bool any_modifier, const std::string &id) {
  auto *display = static_cast<Display *>(x11_display_);
  if (!display || !code) {
    return;
  }
  const Window root = DefaultRootWindow(display);
  if (any_modifier) {
    XGrabKey(display, static_cast<int>(code), AnyModifier, root, True, GrabModeAsync, GrabModeAsync);
    grabs_.push_back({code, static_cast<unsigned>(AnyModifier)});
  } else {
    const unsigned variants[] = {modifiers, modifiers | LockMask, modifiers | Mod2Mask, modifiers | LockMask | Mod2Mask};
    for (unsigned mask : variants) {
      XGrabKey(display, static_cast<int>(code), mask, root, True, GrabModeAsync, GrabModeAsync);
      grabs_.push_back({code, mask});
    }
  }
  bindings_.push_back({code, modifiers, any_modifier, id});
}
#endif

bool GlobalShortcutsBackendX11::DoRegister() {
#ifdef HAVE_X11
  Display *display = XOpenDisplay(nullptr);
  if (!display) {
    return false;
  }
  x11_display_ = display;
  bindings_.clear();
  grabs_.clear();

  if (manager_) {
    for (const auto &shortcut : manager_->shortcuts()) {
      if (shortcut->key().empty()) {
        continue;
      }
      const KeySym keysym = static_cast<KeySym>(KeyMapperX11::KeysymFromName(shortcut->key()));
      if (!keysym) {
        continue;
      }
      const KeyCode code = XKeysymToKeycode(display, keysym);
      if (!code) {
        continue;
      }
      const unsigned modifiers = KeyMapperX11::X11ModifiersFromName(shortcut->key());
      const bool any_modifier = modifiers == static_cast<unsigned>(AnyModifier);
      GrabKey(code, any_modifier ? 0 : modifiers, any_modifier, shortcut->id());
    }
  }

  if (!manager_ || !manager_->HasActiveBackend(Type::Gnome)) {
    const struct {
      KeySym keysym;
      const char *id;
    } media_keys[] = {{XF86XK_AudioPlay, "play_pause"},       {XF86XK_AudioPause, "play_pause"},
                      {XF86XK_AudioStop, "stop"},             {XF86XK_AudioNext, "next_track"},
                      {XF86XK_AudioPrev, "prev_track"},       {XF86XK_AudioRaiseVolume, "inc_volume"},
                      {XF86XK_AudioLowerVolume, "dec_volume"}, {XF86XK_AudioMute, "mute"}};
    for (const auto &media : media_keys) {
      const KeyCode code = XKeysymToKeycode(display, media.keysym);
      if (!code) {
        continue;
      }
      bool already = false;
      for (const Binding &binding : bindings_) {
        if (binding.code == code && binding.any_modifier && binding.id == media.id) {
          already = true;
          break;
        }
      }
      if (!already) {
        GrabKey(code, 0, true, media.id);
      }
    }
  }

  XSync(display, False);
  x11_watch_id_ = g_unix_fd_add(ConnectionNumber(display), G_IO_IN, OnX11Fd, this);
  return !bindings_.empty();
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
    for (const Grab &grab : grabs_) {
      XUngrabKey(display, static_cast<KeyCode>(grab.code), grab.modifiers, root);
    }
    XCloseDisplay(display);
    x11_display_ = nullptr;
    grabs_.clear();
    bindings_.clear();
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
    const unsigned state = event.xkey.state & (ShiftMask | ControlMask | Mod1Mask | Mod4Mask);
    const Binding *matched = nullptr;
    for (const Binding &binding : bindings_) {
      if (binding.code != event.xkey.keycode) {
        continue;
      }
      if (binding.any_modifier) {
        if (!matched) {
          matched = &binding;
        }
        continue;
      }
      if (binding.modifiers == state) {
        matched = &binding;
        break;
      }
    }
    if (matched) {
      manager_->Emit(matched->id);
      continue;
    }
    const std::string id = KeyMapperX11::ShortcutIdFromKeysym(XLookupKeysym(&event.xkey, 0));
    if (!id.empty()) {
      manager_->Emit(id);
    }
  }
}
#endif
