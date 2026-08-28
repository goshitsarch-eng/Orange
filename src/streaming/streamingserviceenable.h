#ifndef STRAWBERRY_STREAMINGSERVICEENABLE_H
#define STRAWBERRY_STREAMINGSERVICEENABLE_H

#include "constants/qobuzsettings.h"
#include "constants/spotifysettings.h"
#include "constants/subsonicsettings.h"
#include "constants/tidalsettings.h"
#include "core/settings.h"

#include <string>
#include <vector>

namespace StreamingServiceEnable {

// Qt MainWindow::ReloadAllSettings EnableTab/DisableTab for Tidal/Spotify/Qobuz/Subsonic.

inline const char *GroupFor(const std::string &name) {
  if (name == "Tidal") {
    return TidalSettings::kSettingsGroup;
  }
  if (name == "Spotify") {
    return SpotifySettings::kSettingsGroup;
  }
  if (name == "Qobuz") {
    return QobuzSettings::kSettingsGroup;
  }
  if (name == "Subsonic") {
    return SubsonicSettings::kSettingsGroup;
  }
  return nullptr;
}

inline bool DefaultEnabled(const std::string &name) {
  if (name == "Tidal") {
    return TidalSettings::kDefaultEnabled;
  }
  if (name == "Spotify") {
    return SpotifySettings::kDefaultEnabled;
  }
  if (name == "Qobuz") {
    return QobuzSettings::kDefaultEnabled;
  }
  if (name == "Subsonic") {
    return SubsonicSettings::kDefaultEnabled;
  }
  return true;
}

inline bool IsEnabled(const std::string &name) {
  const char *group = GroupFor(name);
  if (!group) {
    return true;
  }
  Settings settings;
  settings.BeginGroup(group);
  return settings.BoolValue("enabled", DefaultEnabled(name));
}

inline bool ShouldList(bool enabled) { return enabled; }

inline bool ShouldShowStackPage(bool enabled) { return enabled; }

inline bool ShouldRefreshOnSettingsClose() { return true; }

inline std::vector<std::string> EnabledAmong(const std::vector<std::string> &names) {
  std::vector<std::string> enabled;
  for (const std::string &name : names) {
    if (IsEnabled(name)) {
      enabled.push_back(name);
    }
  }
  return enabled;
}

inline std::string SelectVisible(const std::string &current, const std::vector<std::string> &enabled) {
  for (const std::string &name : enabled) {
    if (name == current) {
      return current;
    }
  }
  return enabled.empty() ? std::string() : enabled.front();
}

}  // namespace StreamingServiceEnable

#endif
