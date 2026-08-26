#ifndef STRAWBERRY_KEYMAPPER_X11_H
#define STRAWBERRY_KEYMAPPER_X11_H

#include <string>

namespace QtKey {
constexpr unsigned MediaPlay = 0x01000080;
constexpr unsigned MediaStop = 0x01000081;
constexpr unsigned MediaPrevious = 0x01000082;
constexpr unsigned MediaNext = 0x01000083;
constexpr unsigned MediaPause = 0x01000085;
constexpr unsigned VolumeDown = 0x01000070;
constexpr unsigned VolumeMute = 0x01000071;
constexpr unsigned VolumeUp = 0x01000072;
constexpr unsigned Space = 0x20;
constexpr unsigned F1 = 0x01000030;
constexpr unsigned Escape = 0x01000000;
constexpr unsigned Tab = 0x01000001;
constexpr unsigned Return = 0x01000004;
constexpr unsigned ShiftModifier = 0x02000000;
constexpr unsigned ControlModifier = 0x04000000;
constexpr unsigned AltModifier = 0x08000000;
constexpr unsigned MetaModifier = 0x10000000;
}  // namespace QtKey

class KeyMapperX11 {
 public:
  static unsigned long KeysymFromName(const std::string &name);
  static std::string NameFromKeysym(unsigned long keysym);
  static unsigned X11ModifiersFromName(const std::string &name);
  static unsigned QtKeyFromName(const std::string &name);
  static unsigned QtShortcutToKey(const std::string &shortcut);
  static std::string ShortcutIdFromKeysym(unsigned long keysym);
  static bool IsMediaKeyName(const std::string &name);
};

#endif
