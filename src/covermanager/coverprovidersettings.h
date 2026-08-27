#ifndef STRAWBERRY_COVERPROVIDERSETTINGS_H
#define STRAWBERRY_COVERPROVIDERSETTINGS_H

#include "covermanager/coverprovider.h"
#include "lyrics/lyricsprovidersettings.h"

#include <string>
#include <utility>
#include <vector>

namespace CoverProviderSettings {

inline const char *ProvidersGroup() { return "Cover providers"; }
inline const char *ProvidersHint() { return "Choose the providers you want to use when searching for covers."; }
inline const char *MoveUp() { return "Move up"; }
inline const char *MoveDown() { return "Move down"; }

inline bool InList(const std::vector<std::string> &names, const std::string &name) { return LyricsProviderSettings::InList(names, name); }

inline bool EnabledFromStored(bool has_name_key, bool name_enabled, bool has_providers_list, bool in_providers_list, bool fallback = true) {
  return LyricsProviderSettings::EnabledFromStored(has_name_key, name_enabled, has_providers_list, in_providers_list, fallback);
}

inline std::vector<std::string> EnabledNames(const std::vector<std::pair<std::string, bool>> &providers) {
  return LyricsProviderSettings::EnabledNames(providers);
}

inline std::vector<std::string> EnabledNames(const std::vector<CoverProvider *> &providers) {
  std::vector<std::pair<std::string, bool>> named;
  named.reserve(providers.size());
  for (CoverProvider *provider : providers) {
    if (provider) {
      named.emplace_back(provider->name(), provider->enabled());
    }
  }
  return EnabledNames(named);
}

}  // namespace CoverProviderSettings

#endif
