#ifndef STRAWBERRY_SONGLOADURL_H
#define STRAWBERRY_SONGLOADURL_H

#include "core/commandlineurl.h"

#include <string>

namespace SongLoadUrl {

// Qt SongLoader::sRawUriSchemes.
inline bool IsRawStreamScheme(const std::string &scheme) {
  return scheme == "udp" || scheme == "rtsp" || scheme == "rtspu" || scheme == "rtspt" || scheme == "rtsph";
}

inline bool ShouldAddAsRawStream(const std::string &url) {
  const std::string scheme = CommandlineUrl::Scheme(url);
  return IsRawStreamScheme(scheme) || scheme == "http" || scheme == "https" || scheme == "mms" || scheme == "mmsh";
}

}  // namespace SongLoadUrl

#endif
