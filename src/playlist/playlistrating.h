#ifndef STRAWBERRY_PLAYLISTRATING_H
#define STRAWBERRY_PLAYLISTRATING_H

#include "core/song.h"

#include <algorithm>
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

// Qt PlaylistView: if the clicked rating cell is already selected, rate the whole selection.
inline std::vector<int> RowsForStarClick(int clicked_row, const std::vector<int> &selected_rows) {
  if (clicked_row < 0) {
    return {};
  }
  if (std::find(selected_rows.begin(), selected_rows.end(), clicked_row) == selected_rows.end()) {
    return {clicked_row};
  }
  return selected_rows;
}

}  // namespace PlaylistRating

#endif
