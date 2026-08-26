#ifndef STRAWBERRY_CONTEXTFORMATTOKENS_H
#define STRAWBERRY_CONTEXTFORMATTOKENS_H

#include <string>
#include <utility>
#include <vector>

namespace ContextFormatTokens {

inline const std::vector<std::pair<std::string, std::string>> &All() {
  static const std::vector<std::pair<std::string, std::string>> tokens = {
      {"%title%", "Title"},
      {"%artist%", "Artist"},
      {"%album%", "Album"},
      {"%albumartist%", "Album artist"},
      {"%track%", "Track"},
      {"%disc%", "Disc"},
      {"%year%", "Year"},
      {"%genre%", "Genre"},
      {"%composer%", "Composer"},
      {"%performer%", "Performer"},
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
