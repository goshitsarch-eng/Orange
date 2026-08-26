#ifndef STRAWBERRY_COVERPROVIDERAUTH_H
#define STRAWBERRY_COVERPROVIDERAUTH_H

#include "constants/qobuzsettings.h"
#include "constants/spotifysettings.h"
#include "constants/tidalsettings.h"
#include "core/settings.h"
#include "settings/settingspages.h"

#include <cstring>
#include <string>
#include <vector>

namespace CoverProviderAuth {

enum class Mode { None, ServiceSettings, Direct };

inline std::vector<const char *> ServiceSettingsProviders() { return {"Tidal", "Spotify", "Qobuz"}; }

inline bool UsesServiceSettings(const std::string &name) {
  for (const char *provider : ServiceSettingsProviders()) {
    if (name == provider) {
      return true;
    }
  }
  return false;
}

inline Mode ModeFor(const std::string &name) { return UsesServiceSettings(name) ? Mode::ServiceSettings : Mode::None; }

inline bool RequiresAuthentication(const std::string &name) { return ModeFor(name) != Mode::None; }

inline bool HasServiceToken(const std::string &name) {
  Settings settings;
  if (name == "Tidal") {
    settings.BeginGroup(TidalSettings::kSettingsGroup);
    return !settings.Value("token").empty() || !settings.Value("access_token").empty();
  }
  if (name == "Spotify") {
    settings.BeginGroup(SpotifySettings::kSettingsGroup);
    return !settings.Value("token").empty() || !settings.Value("access_token").empty();
  }
  if (name == "Qobuz") {
    settings.BeginGroup(QobuzSettings::kSettingsGroup);
    return !settings.Value("token").empty() || !settings.Value("user_auth_token").empty();
  }
  return false;
}

inline std::string StatusText(const std::string &name, bool authenticated) {
  if (name.empty()) {
    return "No cover provider selected.";
  }
  if (!RequiresAuthentication(name)) {
    return name + " does not need authentication.";
  }
  if (UsesServiceSettings(name) && !authenticated) {
    return "Use " + name + " settings to authenticate.";
  }
  return name + " needs authentication.";
}

inline const char *SettingsPageName(const std::string &name) { return SettingsPages::ForService(name); }

inline bool ShowOpenSettings(const std::string &name) { return UsesServiceSettings(name) && SettingsPages::CanOpenAt(SettingsPageName(name)); }

inline std::string OpenSettingsLabel(const std::string &name) { return "Open " + name + " settings"; }

}  // namespace CoverProviderAuth

#endif
