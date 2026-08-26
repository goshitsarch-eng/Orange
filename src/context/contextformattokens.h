#ifndef STRAWBERRY_CONTEXTFORMATTOKENS_H
#define STRAWBERRY_CONTEXTFORMATTOKENS_H

#include <string>
#include <utility>
#include <vector>

namespace ContextFormatTokens {

inline const std::vector<std::pair<std::string, std::string>> &All() {
  static const std::vector<std::pair<std::string, std::string>> tokens = {
      {"%artist%", "Artist"},
      {"%album%", "Album"},
      {"%title%", "Title"},
      {"%albumartist%", "Album artist"},
      {"%year%", "Year"},
      {"%composer%", "Composer"},
      {"%performer%", "Performer"},
      {"%grouping%", "Grouping"},
      {"%disc%", "Disc"},
      {"%track%", "Track"},
      {"%genre%", "Genre"},
      {"%length%", "Length"},
      {"%playcount%", "Play count"},
      {"%skipcount%", "Skip count"},
      {"%rating%", "Rating"},
      {"%newline%", "New line"},
      {"%filename%", "Filename"},
      {"%url%", "URL"},
      {"%originalyear%", "Original year"},
  };
  return tokens;
}

inline std::string Insert(const std::string &format, const std::string &token) {
  if (token.empty()) {
    return format;
  }
  if (format.empty()) {
    return token;
  }
  return format + token;
}

inline bool IsKnown(const std::string &token) {
  for (const auto &entry : All()) {
    if (entry.first == token) {
      return true;
    }
  }
  return false;
}

}  // namespace ContextFormatTokens

#endif
