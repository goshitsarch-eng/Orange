#ifndef STRAWBERRY_QUEUEUI_H
#define STRAWBERRY_QUEUEUI_H

#include "core/song.h"
#include "utilities/timeutils.h"

#include <cstdint>
#include <string>
#include <vector>

namespace QueueUi {

struct ButtonState {
  bool move_up = false;
  bool move_down = false;
  bool remove = false;
  bool clear = false;
};

inline int64_t TotalLengthNanosec(const SongList &songs) {
  int64_t total = 0;
  for (const Song &song : songs) {
    if (song.length_nanosec() > 0) {
      total += song.length_nanosec();
    }
  }
  return total;
}

inline std::string TrackCountText(int tracks) {
  if (tracks == 1) {
    return "1 track";
  }
  return std::to_string(tracks) + " tracks";
}

inline std::string SummaryText(int tracks, int64_t length_nanosec) {
  std::string summary = TrackCountText(tracks);
  if (length_nanosec > 0) {
    summary += " - [ " + Utilities::WordyTimeNanosec(static_cast<uint64_t>(length_nanosec)) + " ]";
  }
  return summary;
}

inline std::string SummaryText(const SongList &songs) { return SummaryText(static_cast<int>(songs.size()), TotalLengthNanosec(songs)); }

inline const char *MoveDownIcon() { return "go-down-symbolic"; }
inline const char *MoveUpIcon() { return "go-up-symbolic"; }
inline const char *RemoveIcon() { return "list-remove-symbolic"; }
inline const char *ClearIcon() { return "edit-clear-symbolic"; }
inline const char *MoveDownTooltip() { return "Move down"; }
inline const char *MoveUpTooltip() { return "Move up"; }
inline const char *RemoveTooltip() { return "Remove"; }
inline const char *ClearTooltip() { return "Clear"; }

inline constexpr const char *kRowClass = "queue-row";
inline constexpr const char *kAltClass = "queue-alt";

inline bool IsAltRow(int index) { return index % 2 == 1; }

inline std::string AlternatingCss() { return ".queue-row.queue-alt { background-color: alpha(currentColor, 0.06); }"; }

inline ButtonState Buttons(const std::vector<int> &selected, int count) {
  ButtonState state;
  state.clear = count > 0;
  if (selected.empty() || count <= 0) {
    return state;
  }
  state.remove = true;
  const bool all_selected = static_cast<int>(selected.size()) == count;
  bool has_first = false;
  bool has_last = false;
  for (int index : selected) {
    if (index == 0) {
      has_first = true;
    }
    if (index == count - 1) {
      has_last = true;
    }
  }
  state.move_up = !all_selected && !has_first;
  state.move_down = !all_selected && !has_last;
  return state;
}

}  // namespace QueueUi

#endif
