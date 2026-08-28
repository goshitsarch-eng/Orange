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

// Qt SystemTrayIcon::MuteButtonStateChanged maps volume==0 onto action_mute_->setChecked.
inline bool IsToggleId(int id, int mute_id) { return id == mute_id; }

inline bool ItemChecked(int id, int mute_id, bool muted) { return id == mute_id && muted; }

inline const char *ToggleType() { return "checkmark"; }

inline int ToggleState(bool muted) { return muted ? 1 : 0; }

inline int ToggleStateForId(int id, int mute_id, bool muted) { return IsToggleId(id, mute_id) ? ToggleState(muted) : -2; }

}  // namespace TrayMenuMute

#endif
