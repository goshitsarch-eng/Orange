#ifndef STRAWBERRY_BEHAVIOURSTARTUPCHOICES_H
#define STRAWBERRY_BEHAVIOURSTARTUPCHOICES_H

#include "constants/behavioursettings.h"

#include <string>
#include <utility>
#include <vector>

namespace BehaviourStartupChoices {

inline bool IncludesHide(bool tray_available, bool tray_enabled) { return tray_available && tray_enabled; }

inline bool TrayDependentSensitive(bool tray_available, bool show_tray) { return tray_available && show_tray; }

inline std::vector<std::pair<std::string, std::string>> StartupChoices(bool tray_available, bool tray_enabled) {
  std::vector<std::pair<std::string, std::string>> choices = {{"1", "Remember"}, {"2", "Show"}};
  if (IncludesHide(tray_available, tray_enabled)) {
    choices.emplace_back("3", "Hide");
  }
  choices.emplace_back("4", "Show maximized");
  choices.emplace_back("5", "Show minimized");
  return choices;
}

inline std::string EffectiveStartup(const std::string &stored, bool tray_available, bool tray_enabled) {
  if (stored == "3" && !IncludesHide(tray_available, tray_enabled)) {
    return std::to_string(static_cast<int>(BehaviourSettings::StartupBehaviour::Remember));
  }
  return stored.empty() ? std::to_string(static_cast<int>(BehaviourSettings::kDefaultStartupBehaviour)) : stored;
}

}  // namespace BehaviourStartupChoices

#endif
