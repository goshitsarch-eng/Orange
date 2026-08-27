#ifndef STRAWBERRY_TRAYMENUPOSITION_H
#define STRAWBERRY_TRAYMENUPOSITION_H

#include <algorithm>

namespace TrayMenuPosition {

struct Rect {
  int x = 0;
  int y = 0;
  int width = 1;
  int height = 1;
};

inline Rect AnchorPoint(int x, int y, int size = 1) {
  const int edge = std::max(1, size);
  return {x, y, edge, edge};
}

inline Rect ClampToMonitor(const Rect &anchor, const Rect &monitor) {
  Rect result = anchor;
  if (result.width > monitor.width) {
    result.width = monitor.width;
  }
  if (result.height > monitor.height) {
    result.height = monitor.height;
  }
  result.x = std::min(std::max(anchor.x, monitor.x), monitor.x + monitor.width - result.width);
  result.y = std::min(std::max(anchor.y, monitor.y), monitor.y + monitor.height - result.height);
  return result;
}

inline bool HasScreenPoint(int x, int y) { return x != 0 || y != 0; }

}  // namespace TrayMenuPosition

#endif  // STRAWBERRY_TRAYMENUPOSITION_H
