#ifndef COLLECTIONEMPTY_H
#define COLLECTIONEMPTY_H

#include "config.h"

#include "collection/collectiontreeclick.h"

namespace CollectionEmpty {

inline constexpr const char *kResourcePath = "/org/strawberrymusicplayer/Strawberry/pictures/nomusic.png";

inline const char *Title() { return "Your collection is empty!"; }
inline const char *Hint() { return "Click here to add some music"; }
inline const char *StreamingTitle() { return "The streaming collection is empty!"; }
inline const char *StreamingHint() { return "Click here to retrieve music"; }
inline const char *NoMatches() { return "No matches"; }

inline bool IsEmptyCollection(const int total_songs) { return total_songs == 0; }

inline bool OpensOnPrimaryClick(const bool empty_collection, const guint button) {
  return empty_collection && button == CollectionTreeClick::kPrimaryButton;
}

}  // namespace CollectionEmpty

#endif  // COLLECTIONEMPTY_H
