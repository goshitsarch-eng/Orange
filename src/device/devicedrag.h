#ifndef STRAWBERRY_DEVICEDRAG_H
#define STRAWBERRY_DEVICEDRAG_H

#include "core/song.h"

#include <string>

namespace DeviceDrag {

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

}  // namespace DeviceDrag

#endif
