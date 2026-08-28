#ifndef STRAWBERRY_PLAYLISTSELECTION_H
#define STRAWBERRY_PLAYLISTSELECTION_H

#include <algorithm>
#include <vector>

// How a click or a cursor key changes the playlist selection.
namespace PlaylistSelection {

enum class Mode {
  // No modifier: the clicked row becomes the whole selection.
  Replace,
  // Control: the clicked row joins or leaves the selection.
  Toggle,
  // Shift: everything between the anchor and the clicked row is selected.
  Range,
};

inline Mode ModeFor(bool control, bool shift) {
  if (shift) {
    return Mode::Range;
  }
  return control ? Mode::Toggle : Mode::Replace;
}

// The rows from the anchor to the target inclusive, in ascending order.  With no anchor the target alone is
// the selection, which is what a shift-click into an empty selection should do.
inline std::vector<int> Range(int anchor, int target) {
  if (target < 0) {
    return {};
  }
  if (anchor < 0) {
    return {target};
  }
  const int first = std::min(anchor, target);
  const int last = std::max(anchor, target);
  std::vector<int> rows;
  rows.reserve(static_cast<size_t>(last - first + 1));
  for (int row = first; row <= last; ++row) {
    rows.push_back(row);
  }
  return rows;
}

// The result of applying a mode to the current selection.
inline std::vector<int> Apply(const std::vector<int> &current, int anchor, int target, Mode mode) {
  if (target < 0) {
    return current;
  }
  if (mode == Mode::Range) {
    return Range(anchor, target);
  }
  if (mode == Mode::Replace) {
    return {target};
  }
  std::vector<int> rows = current;
  const auto it = std::find(rows.begin(), rows.end(), target);
  if (it == rows.end()) {
    rows.push_back(target);
    std::sort(rows.begin(), rows.end());
  }
  else {
    rows.erase(it);
  }
  return rows;
}

// Shift extends from the existing anchor; every other mode makes the clicked row the new anchor.
inline int NextAnchor(int anchor, int target, Mode mode) { return mode == Mode::Range ? anchor : target; }

}  // namespace PlaylistSelection

#endif
