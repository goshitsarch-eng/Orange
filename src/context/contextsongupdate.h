#ifndef STRAWBERRY_CONTEXTSONGUPDATE_H
#define STRAWBERRY_CONTEXTSONGUPDATE_H

#include "core/song.h"

namespace ContextSongUpdate {

// Qt ContextView::SongChanged uses UpdateSong when the play page is showing the same track.
inline bool IsMinorMetadataUpdate(const Song &current, const Song &incoming, bool showing_playing) {
  return showing_playing && current.is_valid() && incoming == current && incoming.title() == current.title() &&
         incoming.album() == current.album() && incoming.artist() == current.artist();
}

inline bool ShouldResetLyrics(bool minor) { return !minor; }

inline bool ShouldResetSearch(bool minor) { return !minor; }

}  // namespace ContextSongUpdate

#endif
