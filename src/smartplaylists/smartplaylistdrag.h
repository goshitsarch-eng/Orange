#ifndef STRAWBERRY_SMARTPLAYLISTDRAG_H
#define STRAWBERRY_SMARTPLAYLISTDRAG_H

#include "core/song.h"
#include "streaming/streamingdrag.h"

#include <string>

namespace SmartPlaylistDrag {

inline std::string DragPayload(const SongList &songs) { return StreamingDrag::DragPayload(songs); }

inline bool CanDrag(bool is_wizard) { return !is_wizard; }

}  // namespace SmartPlaylistDrag

#endif
