#ifndef STRAWBERRY_PLAYERMETADATASYNC_H
#define STRAWBERRY_PLAYERMETADATASYNC_H

#include "core/song.h"

namespace PlayerMetadataSync {

inline void Merge(Song *target, const Song &engine) {
  if (!target) {
    return;
  }
  if (!engine.title().empty()) {
    target->set_title(engine.title());
  }
  if (!engine.artist().empty()) {
    target->set_artist(engine.artist());
  }
  if (!engine.album().empty()) {
    target->set_album(engine.album());
  }
  if (!engine.genre().empty() && target->genre().empty()) {
    target->set_genre(engine.genre());
  }
  if (engine.length_nanosec() > 0 && target->length_nanosec() <= 0) {
    target->set_length_nanosec(engine.length_nanosec());
  }
  if (engine.bitrate() > 0) {
    target->set_bitrate(engine.bitrate());
  }
  if (engine.samplerate() > 0) {
    target->set_samplerate(engine.samplerate());
  }
  if (target->url().empty() && !engine.url().empty()) {
    target->set_url(engine.url());
  }
  if (!engine.stream_url().empty() && engine.stream_url() != engine.url()) {
    target->set_stream_url(engine.stream_url());
  }
}

inline bool ShouldRefreshPlaylist(const Song &before, const Song &after) {
  return before.title() != after.title() || before.artist() != after.artist() || before.album() != after.album() ||
         before.length_nanosec() != after.length_nanosec() || before.stream_url() != after.stream_url();
}

}  // namespace PlayerMetadataSync

#endif  // STRAWBERRY_PLAYERMETADATASYNC_H
