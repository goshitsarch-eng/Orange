#ifndef STRAWBERRY_GLOBALSHORTCUTSPORTAL_H
#define STRAWBERRY_GLOBALSHORTCUTSPORTAL_H

#include "globalshortcuts/globalshortcuts.h"

#include <string>
#include <vector>

namespace GlobalShortcutsPortal {

inline std::string Accelerator(const std::string &key) {
  if (key.empty()) {
    return {};
  }
  if (key.rfind("Media", 0) == 0) {
    if (key == "MediaPlay") {
      return "XF86AudioPlay";
    }
    if (key == "MediaStop") {
      return "XF86AudioStop";
    }
    if (key == "MediaNext") {
      return "XF86AudioNext";
    }
    if (key == "MediaPrevious") {
      return "XF86AudioPrev";
    }
  }
  return key;
}

inline std::vector<std::pair<std::string, std::string>> Bindings(const GlobalShortcutsManager &manager) {
  std::vector<std::pair<std::string, std::string>> bindings;
  for (const auto &shortcut : manager.shortcuts()) {
    if (!shortcut) {
      continue;
    }
    const std::string accel = Accelerator(shortcut->key().empty() ? shortcut->default_key() : shortcut->key());
    if (accel.empty()) {
      continue;
    }
    bindings.emplace_back(shortcut->id(), accel);
  }
  return bindings;
}

}  // namespace GlobalShortcutsPortal

#endif
