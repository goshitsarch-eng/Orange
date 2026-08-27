#ifndef STRAWBERRY_PLAYLISTDYNAMICADVANCE_H
#define STRAWBERRY_PLAYLISTDYNAMICADVANCE_H

#include "core/song.h"
#include "playlist/dynamicplaylistmaintenance.h"
#include "smartplaylists/playlistgenerator.h"

#include <algorithm>
#include <string>
#include <vector>

namespace PlaylistDynamicAdvance {

inline bool ShouldReplenish(bool dynamic, bool advanced_forward) {
  return DynamicPlaylistMaintenance::ShouldClearUndo(dynamic, advanced_forward);
}

inline int ReplenishCount(int history_length, int max_future, int row_count_after_trim) {
  return DynamicPlaylistMaintenance::FutureInsertCount(history_length, max_future, row_count_after_trim);
}

inline std::vector<int> ExistingIds(const SongList &songs) {
  std::vector<int> ids;
  for (const Song &song : songs) {
    if (song.id() > 0) {
      ids.push_back(song.id());
    }
  }
  return ids;
}

inline bool AlreadyPresent(const Song &candidate, const SongList &existing) {
  for (const Song &song : existing) {
    if (candidate.id() > 0 && song.id() == candidate.id()) {
      return true;
    }
    if (!candidate.url().empty() && song.url() == candidate.url() && candidate.beginning_nanosec() == song.beginning_nanosec()) {
      return true;
    }
  }
  return false;
}

inline SongList DedupByIdThenUrl(const SongList &candidates, const SongList &existing) {
  SongList fresh;
  for (const Song &song : candidates) {
    if (!AlreadyPresent(song, existing) && !AlreadyPresent(song, fresh)) {
      fresh.push_back(song);
    }
  }
  return fresh;
}

inline int MaxHistory(const PlaylistGenerator *generator) {
  return generator ? generator->GetDynamicHistory() : PlaylistGenerator::kDefaultDynamicHistory;
}

inline int MaxFuture(const PlaylistGenerator *generator) {
  return generator ? generator->GetDynamicFuture() : PlaylistGenerator::kDefaultDynamicFuture;
}

}  // namespace PlaylistDynamicAdvance

#endif  // STRAWBERRY_PLAYLISTDYNAMICADVANCE_H
