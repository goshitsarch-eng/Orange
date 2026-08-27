#ifndef STRAWBERRY_SCROBBLERSOURCES_H
#define STRAWBERRY_SCROBBLERSOURCES_H

#include "core/song.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace ScrobblerSources {

inline std::vector<Song::Source> All() {
  return {Song::Source::LocalFile, Song::Source::Collection, Song::Source::CDDA,         Song::Source::Device,
          Song::Source::Stream,    Song::Source::Tidal,      Song::Source::Subsonic,     Song::Source::Qobuz,
          Song::Source::SomaFM,    Song::Source::RadioParadise, Song::Source::Spotify,   Song::Source::RadioBrowser};
}

inline std::vector<int> Parse(const std::string &csv) {
  std::vector<int> sources;
  for (const std::string &part : StrUtils::Split(csv, ',')) {
    const std::string token = StrUtils::Trim(part);
    if (token.empty()) {
      continue;
    }
    char *end = nullptr;
    const long value = std::strtol(token.c_str(), &end, 10);
    if (end != token.c_str() && end && *end == '\0') {
      sources.push_back(static_cast<int>(value));
    }
  }
  return sources;
}

inline std::string Join(const std::vector<int> &sources) {
  std::vector<std::string> parts;
  parts.reserve(sources.size());
  for (int source : sources) {
    parts.push_back(std::to_string(source));
  }
  return StrUtils::Join(parts, ",");
}

inline bool Allows(const std::string &csv, Song::Source source) {
  const std::vector<int> sources = Parse(csv);
  if (sources.empty()) {
    return true;
  }
  return std::find(sources.begin(), sources.end(), static_cast<int>(source)) != sources.end();
}

}  // namespace ScrobblerSources

#endif
