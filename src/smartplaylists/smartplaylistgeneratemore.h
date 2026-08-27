#ifndef STRAWBERRY_SMARTPLAYLISTGENERATEMORE_H
#define STRAWBERRY_SMARTPLAYLISTGENERATEMORE_H

#include "core/song.h"
#include "smartplaylists/smartplaylist.h"

#include <algorithm>
#include <vector>

namespace SmartPlaylistGenerateMore {

inline SmartPlaylistSearch Prepare(const SmartPlaylistSearch &search, const std::vector<int> &previous_ids, int current_pos,
                                   int count) {
  SmartPlaylistSearch more = search;
  more.id_not_in = previous_ids;
  if (count > 0) {
    more.limit = count;
  }
  if (!more.sort_random) {
    more.first_item = current_pos;
  }
  return more;
}

inline SongList FilterFresh(const SongList &songs, const std::vector<int> &previous_ids) {
  SongList fresh;
  for (const Song &song : songs) {
    if (song.id() > 0 && std::find(previous_ids.begin(), previous_ids.end(), song.id()) != previous_ids.end()) {
      continue;
    }
    fresh.push_back(song);
  }
  return fresh;
}

inline std::vector<int> TrimHistory(std::vector<int> ids, int max_keep) {
  if (max_keep > 0 && static_cast<int>(ids.size()) > max_keep) {
    ids.erase(ids.begin(), ids.begin() + (static_cast<int>(ids.size()) - max_keep));
  }
  return ids;
}

inline int NextPosition(int current_pos, int limit, bool sort_random) {
  if (sort_random) {
    return current_pos;
  }
  return current_pos + std::max(0, limit);
}

}  // namespace SmartPlaylistGenerateMore

#endif  // STRAWBERRY_SMARTPLAYLISTGENERATEMORE_H
