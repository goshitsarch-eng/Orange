#ifndef STRAWBERRY_STREAMINGSEARCHOPTS_H
#define STRAWBERRY_STREAMINGSEARCHOPTS_H

#include "core/settings.h"
#include "streaming/streamingservice.h"

#include <string>

namespace StreamingSearchOpts {

constexpr char kSearchDelay[] = "searchdelay";
constexpr char kArtistsSearchLimit[] = "artistssearchlimit";
constexpr char kAlbumsSearchLimit[] = "albumssearchlimit";
constexpr char kSongsSearchLimit[] = "songssearchlimit";
constexpr int kDefaultDelayMs = 1500;
constexpr int kDefaultArtistsLimit = 4;
constexpr int kDefaultAlbumsLimit = 10;
constexpr int kDefaultSongsLimit = 10;
constexpr int kMinQueryLength = 2;

inline const char *ConfigureLabel() { return "Configure…"; }

inline bool HasSearchLimits(const std::string &group) {
  return group == "Tidal" || group == "Qobuz" || group == "Spotify";
}

inline int DefaultLimitFor(StreamingService::SearchType type) {
  switch (type) {
    case StreamingService::SearchType::Artists:
      return kDefaultArtistsLimit;
    case StreamingService::SearchType::Albums:
      return kDefaultAlbumsLimit;
    case StreamingService::SearchType::Songs:
      return kDefaultSongsLimit;
  }
  return kDefaultSongsLimit;
}

inline const char *LimitKey(StreamingService::SearchType type) {
  switch (type) {
    case StreamingService::SearchType::Artists:
      return kArtistsSearchLimit;
    case StreamingService::SearchType::Albums:
      return kAlbumsSearchLimit;
    case StreamingService::SearchType::Songs:
      return kSongsSearchLimit;
  }
  return kSongsSearchLimit;
}

inline int ClampDelay(int ms) { return ms < 0 ? 0 : ms; }

inline int ClampLimit(int limit, int fallback) { return limit > 0 ? limit : fallback; }

inline bool ShouldDelay(int delay_ms, bool immediate) { return delay_ms > 0 && !immediate; }

inline bool ShouldSearchOnChange(const std::string &query) { return query.size() >= static_cast<size_t>(kMinQueryLength); }

inline int DelayMs(const std::string &group) {
  Settings settings;
  settings.BeginGroup(group);
  return ClampDelay(settings.IntValue(kSearchDelay, HasSearchLimits(group) ? kDefaultDelayMs : 0));
}

inline int LimitFor(const std::string &group, StreamingService::SearchType type) {
  const int fallback = HasSearchLimits(group) ? DefaultLimitFor(type) : 50;
  Settings settings;
  settings.BeginGroup(group);
  return ClampLimit(settings.IntValue(LimitKey(type), fallback), fallback);
}

}  // namespace StreamingSearchOpts

#endif
