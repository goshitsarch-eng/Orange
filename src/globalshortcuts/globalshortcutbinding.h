#ifndef STRAWBERRY_GLOBALSHORTCUTBINDING_H
#define STRAWBERRY_GLOBALSHORTCUTBINDING_H

#include <string>

namespace GlobalShortcutBinding {

enum class Mode { None = 0, Default = 1, Custom = 2 };

inline const char *NoneLabel() { return "None"; }
inline const char *DefaultLabel() { return "Default"; }
inline const char *CustomLabel() { return "Custom"; }
inline const char *ChangeShortcut() { return "Change shortcut..."; }
inline const char *PageTitle() { return "Global Shortcuts"; }
inline const char *ShortcutColumn() { return "Shortcut"; }

inline std::string ShortcutFor(const std::string &action) {
  if (action.empty()) {
    return "Shortcut";
  }
  return "Shortcut for " + action;
}
inline const char *UseKGlobalAccel() { return "Use KGlobalAccel shortcuts when available"; }
inline const char *UseX11() { return "Use X11 shortcuts when available"; }
inline const char *X11Warning() {
  return "Using X11 shortcuts is not recommended and can cause keyboard to become unresponsive! Shortcuts on should usually be used through MPRIS2 / KGlobalAccel.";
}

inline Mode FromStored(const std::string &stored, const std::string &default_key) {
  if (stored == default_key) {
    return Mode::Default;
  }
  if (stored.empty()) {
    return Mode::None;
  }
  return Mode::Custom;
}

inline Mode FromSettings(bool contains, const std::string &stored, const std::string &default_key) {
  return FromStored(contains ? stored : default_key, default_key);
}

inline std::string ResolveStoredKey(bool contains, const std::string &stored, bool alias_contains, const std::string &alias_stored,
                                    const std::string &default_key) {
  if (contains) {
    return stored;
  }
  if (alias_contains) {
    return alias_stored;
  }
  return default_key;
}

inline Mode FromIndex(int index) {
  switch (index) {
    case 0:
      return Mode::None;
    case 2:
      return Mode::Custom;
    case 1:
    default:
      return Mode::Default;
  }
}

inline int IndexOf(Mode mode) { return static_cast<int>(mode); }

inline bool CustomEnabled(Mode mode) { return mode == Mode::Custom; }

inline std::string EffectiveKey(Mode mode, const std::string &custom, const std::string &default_key) {
  switch (mode) {
    case Mode::None:
      return {};
    case Mode::Custom:
      return custom;
    case Mode::Default:
    default:
      return default_key;
  }
}

inline std::string StoredOrDefault(bool contains, const std::string &stored, const std::string &default_key) {
  return contains ? stored : default_key;
}

}  // namespace GlobalShortcutBinding

#endif
