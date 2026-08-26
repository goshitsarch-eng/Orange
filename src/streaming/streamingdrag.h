#ifndef STRAWBERRY_STREAMINGDRAG_H
#define STRAWBERRY_STREAMINGDRAG_H

#include "core/song.h"

#include <string>

namespace StreamingDrag {

inline std::string DragPayload(const SongList &songs) {
  std::string text;
  for (const Song &song : songs) {
    if (song.url().empty()) {
      continue;
    }
    if (!text.empty()) {
      text += "\n";
    }
    text += song.url();
  }
  return text;
}

}  // namespace StreamingDrag

#endif
