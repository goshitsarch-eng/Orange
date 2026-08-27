#ifndef STRAWBERRY_PLAYLISTCROSSUNDOPAIR_H
#define STRAWBERRY_PLAYLISTCROSSUNDOPAIR_H

#include "playlist/playlist.h"
#include "playlist/playlistundolimits.h"

namespace PlaylistCrossUndoPair {

inline bool ShouldPairUndo(int source_id, int dest_id) { return source_id >= 0 && dest_id >= 0 && source_id != dest_id; }

inline bool ShouldBypass(int item_count) { return PlaylistUndoLimits::ShouldBypassUndo(item_count); }

inline bool UndoBoth(Playlist *source, Playlist *dest) {
  if (!source || !dest) {
    return false;
  }
  bool undone = false;
  if (dest->CanUndo()) {
    dest->Undo();
    undone = true;
  }
  if (source->CanUndo()) {
    source->Undo();
    undone = true;
  }
  return undone;
}

}  // namespace PlaylistCrossUndoPair

#endif  // STRAWBERRY_PLAYLISTCROSSUNDOPAIR_H
