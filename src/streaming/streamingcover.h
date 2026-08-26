#ifndef STRAWBERRY_STREAMINGCOVER_H
#define STRAWBERRY_STREAMINGCOVER_H

#include "core/song.h"
#include "utilities/strutils.h"

#include <string>

namespace StreamingCover {

constexpr int kArtHeight = 32;
constexpr char kPrettyCovers[] = "pretty_covers";
constexpr bool kDefaultPrettyCovers = true;
constexpr char kPlaceholderIcon[] = "media-optical-symbolic";

inline std::string CoverUrl(const Song &song) {
  if (!song.art_manual().empty()) {
    return song.art_manual();
  }
  return song.art_automatic();
}

inline bool IsHttpUrl(const std::string &url) {
  return StrUtils::StartsWith(url, "http://") || StrUtils::StartsWith(url, "https://");
}

inline bool IsLocalUrl(const std::string &url) {
  return StrUtils::StartsWith(url, "file://") || (!url.empty() && url.front() == '/');
}

inline bool CanLoad(const std::string &url) { return IsHttpUrl(url) || IsLocalUrl(url); }

inline bool CanLoad(const Song &song) { return CanLoad(CoverUrl(song)); }

inline std::string CacheKey(const Song &song) {
  const std::string url = CoverUrl(song);
  if (!url.empty()) {
    return url;
  }
  return song.url();
}

inline bool ShouldShowThumb(bool pretty_covers) { return pretty_covers; }

}  // namespace StreamingCover

#endif
