#ifndef STRAWBERRY_SETTINGSPAGES_H
#define STRAWBERRY_SETTINGSPAGES_H

#include "core/song.h"

#include <string>

namespace SettingsPages {

inline const char *Collection() { return "Collection"; }
inline const char *Tidal() { return "Tidal"; }
inline const char *Qobuz() { return "Qobuz"; }
inline const char *Spotify() { return "Spotify"; }
inline const char *Subsonic() { return "Subsonic"; }
inline const char *Radio() { return "Radio"; }
inline const char *Context() { return "Context"; }
inline const char *Notifications() { return "Notifications"; }

inline bool CanOpenAt(const char *page) { return page && page[0] != '\0'; }

inline const char *ForService(const std::string &name) {
  if (name == Tidal()) {
    return Tidal();
  }
  if (name == Qobuz()) {
    return Qobuz();
  }
  if (name == Spotify()) {
    return Spotify();
  }
  if (name == Subsonic()) {
    return Subsonic();
  }
  return nullptr;
}

inline const char *ForSource(Song::Source source) {
  switch (source) {
    case Song::Source::Tidal:
      return Tidal();
    case Song::Source::Qobuz:
      return Qobuz();
    case Song::Source::Spotify:
      return Spotify();
    case Song::Source::Subsonic:
      return Subsonic();
    case Song::Source::Collection:
      return Collection();
    case Song::Source::RadioBrowser:
    case Song::Source::SomaFM:
    case Song::Source::RadioParadise:
      return Radio();
    default:
      return nullptr;
  }
}

}  // namespace SettingsPages

#endif
