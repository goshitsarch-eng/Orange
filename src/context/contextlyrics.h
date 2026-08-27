#ifndef STRAWBERRY_CONTEXTLYRICS_H
#define STRAWBERRY_CONTEXTLYRICS_H

#include "core/song.h"

#include <string>

namespace ContextLyrics {

inline std::string Attribution(const std::string &provider) {
  if (provider.empty()) {
    return {};
  }
  return "Source: " + provider;
}

inline const char *NoResultsText() { return "No lyrics found.\n"; }

inline std::string Footer(const std::string &provider) {
  if (provider.empty()) {
    return {};
  }
  return "\n\n(Lyrics from " + provider + ")\n";
}

inline std::string FormatFetched(const std::string &lyrics, const std::string &provider) {
  if (lyrics.empty()) {
    return NoResultsText();
  }
  return lyrics + Footer(provider);
}

inline bool IsNoResults(const std::string &text) { return text == NoResultsText(); }

inline std::string WithoutFooter(const std::string &text) {
  if (IsNoResults(text)) {
    return {};
  }
  const std::string marker = "\n\n(Lyrics from ";
  const size_t pos = text.rfind(marker);
  if (pos == std::string::npos) {
    return text;
  }
  return text.substr(0, pos);
}

inline std::string InitialLyricsFromSong(const Song &song) { return song.lyrics(); }

inline bool ShouldFetchOnline(const std::string &current_lyrics, bool show_lyrics, bool search_lyrics, const Song &song, bool already_tried) {
  return current_lyrics.empty() && show_lyrics && search_lyrics && !song.artist().empty() && !song.title().empty() && !already_tried;
}

}  // namespace ContextLyrics

#endif
