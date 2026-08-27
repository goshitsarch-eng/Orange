#ifndef STRAWBERRY_STREAMSONGMIMEDATA_H
#define STRAWBERRY_STREAMSONGMIMEDATA_H

#include "core/song.h"

#include <string>

struct StreamSongMimeData {
  SongList songs;
  Song::Source source = Song::Source::Stream;
  std::string service_name;
};

#endif
