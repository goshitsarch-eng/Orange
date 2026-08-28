#ifndef STRAWBERRY_KEYMAPPER_WIN_H
#define STRAWBERRY_KEYMAPPER_WIN_H

#include "utilities/strutils.h"

#include <string>

namespace KeyMapperWin {

inline const char *Name() { return "win"; }

inline bool Parse(const std::string &accel, unsigned *modifiers, unsigned *vk) {
  if (modifiers) {
    *modifiers = 0;
  }
  if (vk) {
    *vk = 0;
  }
  if (accel.empty()) {
    return false;
  }
  const std::string text = StrUtils::ToLower(accel);
  unsigned mods = 0;
  if (text.find("<ctrl>") != std::string::npos || text.find("control+") != std::string::npos) {
    mods |= 0x0002;  // MOD_CONTROL
  }
  if (text.find("<alt>") != std::string::npos || text.find("alt+") != std::string::npos) {
    mods |= 0x0001;  // MOD_ALT
  }
  if (text.find("<shift>") != std::string::npos || text.find("shift+") != std::string::npos) {
    mods |= 0x0004;  // MOD_SHIFT
  }
  if (text.find("<super>") != std::string::npos || text.find("win+") != std::string::npos) {
    mods |= 0x0008;  // MOD_WIN
  }
  unsigned key = 0;
  if (text.find("mediaplay") != std::string::npos) {
    key = 0xB3;  // VK_MEDIA_PLAY_PAUSE
  } else if (text.find("mediastop") != std::string::npos) {
    key = 0xB2;
  } else if (text.find("medianext") != std::string::npos) {
    key = 0xB0;
  } else if (text.find("mediaprevious") != std::string::npos) {
    key = 0xB1;
  }
  if (modifiers) {
    *modifiers = mods;
  }
  if (vk) {
    *vk = key;
  }
  return key != 0;
}

}  // namespace KeyMapperWin

#endif
