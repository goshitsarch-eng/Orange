#ifndef STRAWBERRY_RADIODRAG_H
#define STRAWBERRY_RADIODRAG_H

#include "radios/radiochannel.h"
#include "streaming/streamingdrag.h"

#include <string>
#include <vector>

namespace RadioDrag {

inline SongList Songs(const std::vector<RadioChannel> &channels) {
  SongList songs;
  songs.reserve(channels.size());
  for (const RadioChannel &channel : channels) {
    songs.push_back(channel.ToSong());
  }
  return songs;
}

inline std::string DragPayload(const std::vector<RadioChannel> &channels) { return StreamingDrag::DragPayload(Songs(channels)); }

}  // namespace RadioDrag

#endif
