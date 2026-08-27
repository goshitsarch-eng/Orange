#ifndef STRAWBERRY_LYRICSPROVIDERSETTINGS_H
#define STRAWBERRY_LYRICSPROVIDERSETTINGS_H

#include "lyrics/lyricsprovider.h"
#include "lyrics/lyricsproviderorder.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace LyricsProviderSettings {

inline const char *ProvidersGroup() { return "Lyrics providers"; }
inline const char *ProvidersHint() { return "Choose the providers you want to use when searching for lyrics."; }
inline const char *MoveUp() { return "Move up"; }
inline const char *MoveDown() { return "Move down"; }

inline bool InList(const std::vector<std::string> &names, const std::string &name) {
  return std::find(names.begin(), names.end(), name) != names.end();
}

inline bool EnabledFromStored(bool has_name_key, bool name_enabled, bool has_providers_list, bool in_providers_list, bool fallback = true) {
  if (has_name_key) {
    return name_enabled;
  }
  if (has_providers_list) {
    return in_providers_list;
  }
  return fallback;
}

inline std::vector<std::string> EnabledNames(const std::vector<std::pair<std::string, bool>> &providers) {
  std::vector<std::string> names;
  for (const auto &provider : providers) {
    if (provider.second) {
      names.push_back(provider.first);
    }
  }
  return names;
}

inline std::vector<std::string> EnabledNames(const std::vector<LyricsProvider *> &providers) {
  std::vector<std::pair<std::string, bool>> named;
  named.reserve(providers.size());
  for (LyricsProvider *provider : providers) {
    if (provider) {
      named.emplace_back(provider->name(), provider->enabled());
    }
  }
  return EnabledNames(named);
}

}  // namespace LyricsProviderSettings

#endif
