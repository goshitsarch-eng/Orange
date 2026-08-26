#include "globalshortcuts/keymapper_x11.h"

#include "config.h"

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#endif

unsigned long KeyMapperX11::KeysymFromName(const std::string &name) {
#ifdef HAVE_X11
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
#else
  (void)name;
  return 0;
#endif
}

std::string KeyMapperX11::NameFromKeysym(unsigned long keysym) {
#ifdef HAVE_X11
  if (keysym == XF86XK_AudioPlay) return "MediaPlay";
  if (keysym == XF86XK_AudioPause) return "MediaPause";
  if (keysym == XF86XK_AudioStop) return "MediaStop";
  if (keysym == XF86XK_AudioNext) return "MediaNext";
  if (keysym == XF86XK_AudioPrev) return "MediaPrevious";
  if (keysym == XF86XK_AudioRaiseVolume) return "VolumeUp";
  if (keysym == XF86XK_AudioLowerVolume) return "VolumeDown";
  if (keysym == XF86XK_AudioMute) return "Mute";
  if (const char *name = XKeysymToString(keysym)) {
    return name;
  }
#else
  (void)keysym;
#endif
  return {};
}
