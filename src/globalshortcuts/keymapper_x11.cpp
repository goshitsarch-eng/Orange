#include "globalshortcuts/keymapper_x11.h"

#include "config.h"

#ifdef HAVE_X11
#include <X11/Xlib.h>
#include <X11/XF86keysym.h>
#include <X11/keysym.h>
#endif

#include <cctype>

namespace {

std::string Lower(std::string value) {
  for (char &ch : value) {
    ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  }
  return value;
}

struct ParsedShortcut {
  std::string key;
  bool ctrl = false;
  bool shift = false;
  bool alt = false;
  bool meta = false;
  bool has_mod = false;
};

void ApplyModifier(ParsedShortcut *parsed, const std::string &token) {
  const std::string lower = Lower(token);
  if (lower == "ctrl" || lower == "control" || lower == "primary") {
    parsed->ctrl = true;
    parsed->has_mod = true;
  } else if (lower == "shift") {
    parsed->shift = true;
    parsed->has_mod = true;
  } else if (lower == "alt" || lower == "mod1") {
    parsed->alt = true;
    parsed->has_mod = true;
  } else if (lower == "meta" || lower == "super" || lower == "win" || lower == "mod4") {
    parsed->meta = true;
    parsed->has_mod = true;
  } else if (!token.empty()) {
    parsed->key = token;
  }
}

ParsedShortcut ParseShortcutName(const std::string &name) {
  ParsedShortcut parsed;
  std::string token;
  for (size_t i = 0; i <= name.size(); ++i) {
    const char ch = i < name.size() ? name[i] : '+';
    if (ch == '<' ) {
      if (!token.empty()) {
        ApplyModifier(&parsed, token);
        token.clear();
      }
      continue;
    }
    if (ch == '>' || ch == '+' || i == name.size()) {
      if (!token.empty()) {
        ApplyModifier(&parsed, token);
        token.clear();
      }
      continue;
    }
    token.push_back(ch);
  }
  if (parsed.key.empty()) {
    parsed.key = name;
  }
  return parsed;
}

bool IsMediaToken(const std::string &name) {
  const std::string lower = Lower(name);
  return lower == "mediaplay" || lower == "media play" || lower == "xf86audioplay" || lower == "play" ||
         lower == "mediapause" || lower == "media pause" || lower == "xf86audiopause" || lower == "pause" ||
         lower == "mediastop" || lower == "media stop" || lower == "xf86audiostop" || lower == "stop" ||
         lower == "medianext" || lower == "media next" || lower == "xf86audionext" || lower == "next" ||
         lower == "mediaprevious" || lower == "media previous" || lower == "media prev" || lower == "xf86audioprev" ||
         lower == "previous" || lower == "xf86audioraisevolume" || lower == "volumeup" || lower == "volume up" ||
         lower == "xf86audiolowervolume" || lower == "volumedown" || lower == "volume down" || lower == "xf86audiomute" ||
         lower == "volumemute" || lower == "mute";
}

}  // namespace

bool KeyMapperX11::IsMediaKeyName(const std::string &name) { return IsMediaToken(ParseShortcutName(name).key); }

unsigned long KeyMapperX11::KeysymFromName(const std::string &name) {
#ifdef HAVE_X11
  const std::string key = ParseShortcutName(name).key;
  const std::string lower = Lower(key);
  if (key.empty()) {
    return 0;
  }
  if (lower == "mediaplay" || lower == "media play" || lower == "xf86audioplay" || lower == "play") {
    return XF86XK_AudioPlay;
  }
  if (lower == "mediapause" || lower == "media pause" || lower == "xf86audiopause" || lower == "pause") {
    return XF86XK_AudioPause;
  }
  if (lower == "mediastop" || lower == "media stop" || lower == "xf86audiostop" || lower == "stop") {
    return XF86XK_AudioStop;
  }
  if (lower == "medianext" || lower == "media next" || lower == "xf86audionext" || lower == "next") {
    return XF86XK_AudioNext;
  }
  if (lower == "mediaprevious" || lower == "media previous" || lower == "media prev" || lower == "xf86audioprev" ||
      lower == "previous") {
    return XF86XK_AudioPrev;
  }
  if (lower == "xf86audioraisevolume" || lower == "volumeup" || lower == "volume up") {
    return XF86XK_AudioRaiseVolume;
  }
  if (lower == "xf86audiolowervolume" || lower == "volumedown" || lower == "volume down") {
    return XF86XK_AudioLowerVolume;
  }
  if (lower == "xf86audiomute" || lower == "volumemute" || lower == "mute") {
    return XF86XK_AudioMute;
  }
  if (lower == "space") {
    return XK_space;
  }
  return XStringToKeysym(key.c_str());
#else
  (void)name;
  return 0;
#endif
}

unsigned KeyMapperX11::X11ModifiersFromName(const std::string &name) {
#ifdef HAVE_X11
  const ParsedShortcut parsed = ParseShortcutName(name);
  if (!parsed.has_mod) {
    return IsMediaToken(parsed.key) ? static_cast<unsigned>(AnyModifier) : 0;
  }
  unsigned modifiers = 0;
  if (parsed.ctrl) {
    modifiers |= ControlMask;
  }
  if (parsed.shift) {
    modifiers |= ShiftMask;
  }
  if (parsed.alt) {
    modifiers |= Mod1Mask;
  }
  if (parsed.meta) {
    modifiers |= Mod4Mask;
  }
  return modifiers;
#else
  (void)name;
  return 0;
#endif
}

unsigned KeyMapperX11::QtKeyFromName(const std::string &name) {
  const std::string key = ParseShortcutName(name).key;
  const std::string lower = Lower(key);
  if (lower.empty()) {
    return 0;
  }
  if (lower == "mediaplay" || lower == "media play" || lower == "xf86audioplay" || lower == "play") {
    return QtKey::MediaPlay;
  }
  if (lower == "mediapause" || lower == "media pause" || lower == "xf86audiopause") {
    return QtKey::MediaPause;
  }
  if (lower == "mediastop" || lower == "media stop" || lower == "xf86audiostop" || lower == "stop") {
    return QtKey::MediaStop;
  }
  if (lower == "medianext" || lower == "media next" || lower == "xf86audionext" || lower == "next") {
    return QtKey::MediaNext;
  }
  if (lower == "mediaprevious" || lower == "media previous" || lower == "media prev" || lower == "xf86audioprev" ||
      lower == "previous") {
    return QtKey::MediaPrevious;
  }
  if (lower == "xf86audioraisevolume" || lower == "volumeup" || lower == "volume up") {
    return QtKey::VolumeUp;
  }
  if (lower == "xf86audiolowervolume" || lower == "volumedown" || lower == "volume down") {
    return QtKey::VolumeDown;
  }
  if (lower == "xf86audiomute" || lower == "volumemute" || lower == "mute") {
    return QtKey::VolumeMute;
  }
  if (lower == "space") {
    return QtKey::Space;
  }
  if (lower == "escape" || lower == "esc") {
    return QtKey::Escape;
  }
  if (lower == "tab") {
    return QtKey::Tab;
  }
  if (lower == "return" || lower == "enter") {
    return QtKey::Return;
  }
  if (lower.size() >= 2 && (lower[0] == 'f' || lower[0] == 'F') && std::isdigit(static_cast<unsigned char>(lower[1]))) {
    const int function = std::atoi(lower.c_str() + 1);
    if (function >= 1 && function <= 35) {
      return QtKey::F1 + static_cast<unsigned>(function - 1);
    }
  }
  if (key.size() == 1) {
    return static_cast<unsigned>(std::toupper(static_cast<unsigned char>(key[0])));
  }
  return 0;
}

unsigned KeyMapperX11::QtShortcutToKey(const std::string &shortcut) {
  if (shortcut.empty()) {
    return 0;
  }
  const ParsedShortcut parsed = ParseShortcutName(shortcut);
  unsigned key = QtKeyFromName(parsed.key.empty() ? shortcut : parsed.key);
  if (!key) {
    return 0;
  }
  if (parsed.ctrl) {
    key |= QtKey::ControlModifier;
  }
  if (parsed.shift) {
    key |= QtKey::ShiftModifier;
  }
  if (parsed.alt) {
    key |= QtKey::AltModifier;
  }
  if (parsed.meta) {
    key |= QtKey::MetaModifier;
  }
  return key;
}

std::string KeyMapperX11::ShortcutIdFromKeysym(unsigned long keysym) {
#ifdef HAVE_X11
  if (keysym == XF86XK_AudioPlay || keysym == XF86XK_AudioPause) {
    return "play_pause";
  }
  if (keysym == XF86XK_AudioStop) {
    return "stop";
  }
  if (keysym == XF86XK_AudioNext) {
    return "next_track";
  }
  if (keysym == XF86XK_AudioPrev) {
    return "prev_track";
  }
  if (keysym == XF86XK_AudioRaiseVolume) {
    return "inc_volume";
  }
  if (keysym == XF86XK_AudioLowerVolume) {
    return "dec_volume";
  }
  if (keysym == XF86XK_AudioMute) {
    return "mute";
  }
#else
  (void)keysym;
#endif
  return {};
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
