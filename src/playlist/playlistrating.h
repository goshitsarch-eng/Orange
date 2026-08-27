#ifndef STRAWBERRY_PLAYLISTRATING_H
#define STRAWBERRY_PLAYLISTRATING_H

#include "core/song.h"

#include <vector>

namespace PlaylistRating {

// Qt Playlist::RateSong / RateSongs: only local collection items with a real id.
inline bool ShouldWriteCollectionRating(const Song &song) { return song.is_collection_song() && song.id() > 0; }

inline std::vector<int> CollectionIdsToRate(const SongList &songs) {
  std::vector<int> ids;
  for (const Song &song : songs) {
    if (ShouldWriteCollectionRating(song)) {
      ids.push_back(song.id());
    }
  }
  return ids;
}

inline bool ShouldSaveRatingTags(bool save_ratings_in_tags, bool editable) { return save_ratings_in_tags && editable; }

}  // namespace PlaylistRating

#endif
