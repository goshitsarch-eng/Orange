#ifndef STRAWBERRY_TRAYMENUMUTE_H
#define STRAWBERRY_TRAYMENUMUTE_H

#include <algorithm>
#include <vector>

namespace TrayMenuMute {

// Qt SystemTrayIcon::SetMuteEnabled maps BackendSettings::kVolumeControl onto action_mute_->setVisible.
inline bool ShouldShow(bool volume_control) { return volume_control; }

inline std::vector<int> FilterMenuIds(std::vector<int> ids, int mute_id, bool show_mute) {
  if (show_mute) {
    return ids;
  }
  ids.erase(std::remove(ids.begin(), ids.end(), mute_id), ids.end());
  return ids;
}

inline bool ItemVisible(int id, int mute_id, bool mute_visible) { return id != mute_id || mute_visible; }

}  // namespace TrayMenuMute

#endif
