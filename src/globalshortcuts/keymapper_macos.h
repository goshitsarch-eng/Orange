#ifndef STRAWBERRY_KEYMAPPER_MACOS_H
#define STRAWBERRY_KEYMAPPER_MACOS_H

#include "utilities/strutils.h"

#include <string>

namespace KeyMapperMacOs {

inline const char *Name() { return "macos"; }

// IOKit ev_keymap.h NX_KEYTYPE_* values used by NSSystemDefined media keys.
inline constexpr int kNxPlay = 16;
inline constexpr int kNxFast = 17;
inline constexpr int kNxRewind = 18;
inline constexpr int kAuxControlSubtype = 8;
inline constexpr int kKeyDownState = 0x0A;

inline const char *IdFromMediaKey(int nx_key) {
  switch (nx_key) {
    case kNxPlay:
      return "play_pause";
    case kNxFast:
      return "next_track";
    case kNxRewind:
      return "prev_track";
    default:
      return "";
  }
}

inline const char *KeyNameFromMediaKey(int nx_key) {
  switch (nx_key) {
    case kNxPlay:
      return "MediaPlay";
    case kNxFast:
      return "MediaNext";
    case kNxRewind:
      return "MediaPrevious";
    default:
      return "";
  }
}

inline int MediaKeyFromData1(int data1) { return (data1 >> 16) & 0xFFFF; }

inline bool IsMediaKeyDown(int data1) { return ((data1 >> 8) & 0xFF) == kKeyDownState; }

inline bool IsAuxControlEvent(int subtype) { return subtype == kAuxControlSubtype; }

inline const char *ShortcutIdFromKey(const std::string &key) {
  const std::string text = StrUtils::ToLower(key);
  if (text.find("mediaplay") != std::string::npos) {
    return "play_pause";
  }
  if (text.find("mediastop") != std::string::npos) {
    return "stop";
  }
  if (text.find("medianext") != std::string::npos) {
    return "next_track";
  }
  if (text.find("mediaprevious") != std::string::npos) {
    return "prev_track";
  }
  return "";
}

inline bool KeysMatch(const std::string &event_key, const std::string &bound) {
  return !bound.empty() && StrUtils::ToLower(event_key) == StrUtils::ToLower(bound);
}

inline constexpr unsigned kShift = 1u << 17;
inline constexpr unsigned kControl = 1u << 18;
inline constexpr unsigned kOption = 1u << 19;
inline constexpr unsigned kCommand = 1u << 20;

inline std::string AcceleratorFromEvent(unsigned modifiers, const std::string &key) {
  std::string out;
  if (modifiers & kControl) {
    out += "<Ctrl>";
  }
  if (modifiers & kOption) {
    out += "<Alt>";
  }
  if (modifiers & kShift) {
    out += "<Shift>";
  }
  if (modifiers & kCommand) {
    out += "<Super>";
  }
  out += key;
  return out;
}

}  // namespace KeyMapperMacOs

#endif
