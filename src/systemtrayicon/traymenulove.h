#ifndef STRAWBERRY_TRAYMENULOVE_H
#define STRAWBERRY_TRAYMENULOVE_H

#include <algorithm>
#include <vector>

namespace TrayMenuLove {

inline std::vector<int> FilterMenuIds(std::vector<int> ids, int love_id, bool show_love) {
  if (show_love) {
    return ids;
  }
  ids.erase(std::remove(ids.begin(), ids.end(), love_id), ids.end());
  return ids;
}

inline bool LoveEnabled(bool scrobbler_enabled, bool metadata_good, bool loved_this_track) {
  return scrobbler_enabled && metadata_good && !loved_this_track;
}

inline bool ItemEnabled(int id, int love_id, bool love_enabled) { return id != love_id || love_enabled; }

inline bool ItemVisible(int id, int love_id, bool love_visible) { return id != love_id || love_visible; }

}  // namespace TrayMenuLove

#endif
