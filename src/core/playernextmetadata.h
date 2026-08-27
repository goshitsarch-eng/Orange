#ifndef STRAWBERRY_PLAYERNEXTMETADATA_H
#define STRAWBERRY_PLAYERNEXTMETADATA_H

#include "core/song.h"

#include <string>

namespace PlayerNextMetadata {

enum class Target { None, Current, Next };

inline bool UrlMatches(const std::string &meta_url, const std::string &url, const std::string &stream_url) {
  return !meta_url.empty() && (meta_url == url || (!stream_url.empty() && meta_url == stream_url));
}

inline Target TargetForUrl(const std::string &meta_url, const std::string &current_url, const std::string &current_stream,
                           const std::string &next_url, const std::string &next_stream) {
  if (UrlMatches(meta_url, next_url, next_stream) && !UrlMatches(meta_url, current_url, current_stream)) {
    return Target::Next;
  }
  if (meta_url.empty() || UrlMatches(meta_url, current_url, current_stream)) {
    return Target::Current;
  }
  return Target::None;
}

inline bool ShouldApplyToNext(Target target) { return target == Target::Next; }

}  // namespace PlayerNextMetadata

#endif  // STRAWBERRY_PLAYERNEXTMETADATA_H
