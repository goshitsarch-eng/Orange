#ifndef STRAWBERRY_LISTENBRAINZSCROBBLESTATE_H
#define STRAWBERRY_LISTENBRAINZSCROBBLESTATE_H

#include "constants/timeconstants.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cstdint>
#include <string>

namespace ListenBrainzScrobbleState {

// Qt ListenBrainzScrobbler::Submit uses listen_type "import" for cached scrobbles.
inline const char *CachedListenType() { return "import"; }

// Qt StartSubmit: std::max(submit_delay, submit_error ? 30 : 5) — always at least 5s.
inline int DelaySeconds(int configured, bool had_error) { return std::max(configured, had_error ? 30 : 5); }

inline bool ShouldStartSubmitTimer(bool submitted, bool has_unsent, bool timer_active) {
  return !submitted && has_unsent && !timer_active;
}

inline bool ShouldIncludeReleaseName(const std::string &album) { return !album.empty(); }

inline int DurationMs(int64_t length_nanosec) {
  return length_nanosec > 0 ? static_cast<int>(length_nanosec / TimeConstants::kNsecPerMsec) : 0;
}

// Qt JsonTrackMetadata additional_info: duration_ms, tracknumber, media_player, submission_client.
inline std::string AdditionalInfoJson(int track, int64_t length_nanosec, const std::string &version = {}) {
  std::string json = "{\"media_player\":\"Strawberry\",\"submission_client\":\"Strawberry\"";
  if (!version.empty()) {
    const std::string escaped = StrUtils::JsonEscape(version);
    json += ",\"media_player_version\":\"" + escaped + "\",\"submission_client_version\":\"" + escaped + "\"";
  }
  const int duration_ms = DurationMs(length_nanosec);
  if (duration_ms > 0) {
    json += ",\"duration_ms\":" + std::to_string(duration_ms);
  }
  if (track > 0) {
    json += ",\"tracknumber\":" + std::to_string(track);
  }
  json += "}";
  return json;
}

}  // namespace ListenBrainzScrobbleState

#endif
