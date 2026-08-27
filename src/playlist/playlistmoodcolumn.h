#ifndef PLAYLIST_PLAYLISTMOODCOLUMN_H_
#define PLAYLIST_PLAYLISTMOODCOLUMN_H_

#include "playlist/playlistdelegates.h"

namespace PlaylistMoodColumn {

inline bool IsMoodFeatureColumn(PlaylistColumn column) {
  return column == PlaylistColumn::Mood || column == PlaylistColumn::Moodbar;
}

inline bool ShouldOffer(PlaylistColumn column, bool moodbar_available) {
  return moodbar_available || !IsMoodFeatureColumn(column);
}

#ifdef HAVE_MOODBAR
inline constexpr bool kMoodbarAvailable = true;
#else
inline constexpr bool kMoodbarAvailable = false;
#endif

inline bool ShouldOffer(PlaylistColumn column) { return ShouldOffer(column, kMoodbarAvailable); }

}  // namespace PlaylistMoodColumn

#endif  // PLAYLIST_PLAYLISTMOODCOLUMN_H_
