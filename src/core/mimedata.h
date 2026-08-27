#ifndef STRAWBERRY_MIMEDATA_H
#define STRAWBERRY_MIMEDATA_H

#include "core/song.h"

#include <string>

struct MimeData {
  SongList songs;
  std::string urls;
  std::string text() const;
};

#endif
