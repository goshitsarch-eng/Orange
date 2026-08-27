#ifndef STRAWBERRY_SONGSEGMENT_H
#define STRAWBERRY_SONGSEGMENT_H

#include "core/song.h"

#include <cstdint>

namespace SongSegment {

inline int64_t EffectiveEndNanosec(const Song &song) {
  if (song.end_nanosec() > 0) {
    return song.end_nanosec();
  }
  if (song.length_nanosec() > 0) {
    return song.beginning_nanosec() + song.length_nanosec();
  }
  return -1;
}

inline bool HasForcedEnd(const Song &song) { return song.has_cue() || song.end_nanosec() > 0; }

}  // namespace SongSegment

#endif  // STRAWBERRY_SONGSEGMENT_H
