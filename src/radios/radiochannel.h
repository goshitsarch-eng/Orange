#ifndef STRAWBERRY_RADIOCHANNEL_H
#define STRAWBERRY_RADIOCHANNEL_H

#include "core/song.h"

#include <string>

struct RadioChannel {
  std::string name;
  std::string url;
  std::string thumbnail_url;
  std::string country;
  std::string tags;
  std::string codec;
  Song::Source source = Song::Source::Stream;

  Song ToSong() const;
};

#endif
