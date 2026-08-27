#ifndef STRAWBERRY_DISCORDART_H
#define STRAWBERRY_DISCORDART_H

#include <string>

namespace DiscordArt {

inline constexpr const char *kEmbeddedCover = "embedded_cover";

inline bool IsHttpUrl(const std::string &url) {
  return url.rfind("https://", 0) == 0 || url.rfind("http://", 0) == 0;
}

inline std::string ArtKey(const std::string &art_url) { return IsHttpUrl(art_url) ? art_url : kEmbeddedCover; }

inline std::string PresenceArtKey(const std::string &art_url, const std::string &uploaded_data_url) {
  if (IsHttpUrl(art_url)) {
    return art_url;
  }
  if (!uploaded_data_url.empty()) {
    return uploaded_data_url;
  }
  return kEmbeddedCover;
}

inline std::string SongArtUrl(const std::string &manual, const std::string &automatic) { return !manual.empty() ? manual : automatic; }

}  // namespace DiscordArt

#endif
