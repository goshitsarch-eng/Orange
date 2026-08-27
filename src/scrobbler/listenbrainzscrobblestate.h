#ifndef STRAWBERRY_LISTENBRAINZSCROBBLESTATE_H
#define STRAWBERRY_LISTENBRAINZSCROBBLESTATE_H

#include <algorithm>

namespace ListenBrainzScrobbleState {

// Qt ListenBrainzScrobbler::Submit uses listen_type "import" for cached scrobbles.
inline const char *CachedListenType() { return "import"; }

// Qt StartSubmit: std::max(submit_delay, submit_error ? 30 : 5) — always at least 5s.
inline int DelaySeconds(int configured, bool had_error) { return std::max(configured, had_error ? 30 : 5); }

inline bool ShouldStartSubmitTimer(bool submitted, bool has_unsent, bool timer_active) {
  return !submitted && has_unsent && !timer_active;
}

}  // namespace ListenBrainzScrobbleState

#endif
