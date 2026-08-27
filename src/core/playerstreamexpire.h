#ifndef STRAWBERRY_PLAYERSTREAMEXPIRE_H
#define STRAWBERRY_PLAYERSTREAMEXPIRE_H

#include "core/song.h"

#include <cstdint>

namespace PlayerStreamExpire {

inline constexpr int kExpirePauseSeconds = 30;

inline bool NeedsRefresh(const Song &song, bool has_handler, int64_t pause_started_sec, int64_t now_sec) {
  if (!has_handler || !song.stream_url_can_expire() || pause_started_sec <= 0) {
    return false;
  }
  return now_sec - pause_started_sec >= kExpirePauseSeconds;
}

}  // namespace PlayerStreamExpire

#endif  // STRAWBERRY_PLAYERSTREAMEXPIRE_H
