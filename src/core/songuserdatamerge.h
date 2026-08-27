#ifndef STRAWBERRY_SONGUSERDATAMERGE_H
#define STRAWBERRY_SONGUSERDATAMERGE_H

#include "core/song.h"

namespace SongUserDataMerge {

inline void Merge(Song *incoming, const Song &existing, bool merge_playcount, bool merge_rating) {
  if (!incoming) {
    return;
  }
  if (merge_playcount && existing.playcount() > 0) {
    incoming->set_playcount(existing.playcount());
  }
  if (merge_rating && existing.rating() > 0.0f) {
    incoming->set_rating(existing.rating());
  }
  incoming->set_skipcount(existing.skipcount());
  incoming->set_lastplayed(existing.lastplayed());
  incoming->set_art_manual(existing.art_manual());
  incoming->set_art_unset(existing.art_unset());
}

}  // namespace SongUserDataMerge

#endif  // STRAWBERRY_SONGUSERDATAMERGE_H
