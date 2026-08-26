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

inline bool ValidTidalCoverSize(const std::string &size) {
  return size == "160x160" || size == "320x320" || size == "640x640" || size == "1280x1280";
}

inline std::string ClampTidalCoverSize(const std::string &size) { return ValidTidalCoverSize(size) ? size : "640x640"; }

inline std::string WithTidalCoverSize(const std::string &url, const std::string &size) {
  if (!IsHttpUrl(url)) {
    return url;
  }
  const std::string use = ClampTidalCoverSize(size);
  const auto slash = url.rfind('/');
  if (slash == std::string::npos) {
    return url;
  }
  return url.substr(0, slash + 1) + use + ".jpg";
}

inline void ApplyTidalCoverSize(SongList &songs, const std::string &size) {
  const std::string use = ClampTidalCoverSize(size);
  for (Song &song : songs) {
    if (!IsHttpUrl(song.art_automatic())) {
      continue;
    }
    song.set_art_automatic(WithTidalCoverSize(song.art_automatic(), use));
  }
}

}  // namespace StreamingCover

#endif
