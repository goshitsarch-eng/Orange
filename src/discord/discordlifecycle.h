#ifndef STRAWBERRY_DISCORDLIFECYCLE_H
#define STRAWBERRY_DISCORDLIFECYCLE_H

#include <glib.h>

namespace DiscordLifecycle {

inline bool ShouldClear(bool playing) { return !playing; }

inline gint64 StartTimestampAfterSeek(gint64 now_secs, gint64 position_secs) {
  if (position_secs < 0) {
    position_secs = 0;
  }
  return now_secs - position_secs;
}

}  // namespace DiscordLifecycle

#endif  // STRAWBERRY_DISCORDLIFECYCLE_H
