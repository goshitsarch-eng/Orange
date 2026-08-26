#ifndef STRAWBERRY_STREAMINGBROWSE_H
#define STRAWBERRY_STREAMINGBROWSE_H

#include "core/song.h"

namespace StreamingBrowse {

enum class Kind { Artist, Album, Song };

inline Kind KindOf(const Song &song) {
  if (!song.song_id().empty()) {
    return Kind::Song;
  }
  if (!song.album_id().empty()) {
    return Kind::Album;
  }
  if (!song.artist_id().empty()) {
    return Kind::Artist;
  }
  if (song.title().empty() && !song.album().empty()) {
    return Kind::Album;
  }
  if (song.title().empty() && !song.artist().empty()) {
    return Kind::Artist;
  }
  return Kind::Song;
}

inline bool CanBrowse(Kind kind) { return kind != Kind::Song; }

}  // namespace StreamingBrowse

#endif
