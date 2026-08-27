#ifndef STRAWBERRY_OSDPRETTYPLACEMENT_H
#define STRAWBERRY_OSDPRETTYPLACEMENT_H

#include <algorithm>
#include <cstdlib>
#include <string>
#include <vector>

namespace OSDPrettyPlacement {

struct Rect {
  int x = 0;
  int y = 0;
  int width = 0;
  int height = 0;

  int Right() const { return x + width - 1; }
  int Bottom() const { return y + height - 1; }
};

struct Point {
  int x = 0;
  int y = 0;
};

inline int Clamp(int value, int min, int max) { return std::max(min, std::min(value, max)); }

inline bool IsSnappedToCenter(int value, int center, int proximity = 20) {
  return value > center - proximity && value < center + proximity;
}

inline int SnapCenter(int value, int center, int proximity = 20) {
  if (IsSnappedToCenter(value, center, proximity)) {
    return center;
  }
  return value;
}

// Matches Qt OSDPretty::Reposition: negative pos docks to the right or bottom edge.
inline Point AbsolutePosition(const Rect &monitor, Point pos, int width, int height, bool clamp = true) {
  Point abs;
  abs.x = pos.x < 0 ? monitor.Right() - width : monitor.x + pos.x;
  abs.y = pos.y < 0 ? monitor.Bottom() - height : monitor.y + pos.y;
  if (clamp) {
    abs.x = Clamp(abs.x, 0, monitor.Right() - width);
    abs.y = Clamp(abs.y, 0, monitor.Bottom() - height);
  }
  return abs;
}

// Matches Qt OSDPretty::current_pos: a window on the far edge is stored as -1.
inline Point RelativePosition(const Rect &monitor, Point abs, int width, int height) {
  Point pos;
  pos.x = abs.x >= monitor.Right() - width ? -1 : abs.x - monitor.x;
  pos.y = abs.y >= monitor.Bottom() - height ? -1 : abs.y - monitor.y;
  return pos;
}

inline Point DragPosition(const Rect &monitor, Point abs, int width, int height) {
  Point out;
  out.x = Clamp(abs.x, monitor.x, monitor.Right() - width);
  out.y = Clamp(abs.y, monitor.y, monitor.Bottom() - height);
  out.x = SnapCenter(out.x, monitor.x + monitor.width / 2 - width / 2);
  return out;
}

inline int IndexContaining(const std::vector<Rect> &monitors, Point point) {
  for (size_t i = 0; i < monitors.size(); ++i) {
    const Rect &monitor = monitors[i];
    if (point.x >= monitor.x && point.x < monitor.x + monitor.width && point.y >= monitor.y && point.y < monitor.y + monitor.height) {
      return static_cast<int>(i);
    }
  }
  return monitors.empty() ? -1 : 0;
}

// Accepts a connector name or a legacy integer index from older GTK settings.
inline int ResolveIndex(const std::string &screen, const std::vector<std::string> &names) {
  if (names.empty()) {
    return 0;
  }
  if (!screen.empty()) {
    for (size_t i = 0; i < names.size(); ++i) {
      if (names[i] == screen) {
        return static_cast<int>(i);
      }
    }
    char *end = nullptr;
    const long index = std::strtol(screen.c_str(), &end, 10);
    if (end && *end == '\0' && index >= 0 && static_cast<size_t>(index) < names.size()) {
      return static_cast<int>(index);
    }
  }
  return 0;
}

inline Point ParsePos(const std::string &value, Point fallback = {}) {
  if (value.empty()) {
    return fallback;
  }
  const auto comma = value.find(',');
  if (comma == std::string::npos) {
    return fallback;
  }
  return {std::atoi(value.substr(0, comma).c_str()), std::atoi(value.substr(comma + 1).c_str())};
}

inline std::string FormatPos(Point pos) { return std::to_string(pos.x) + "," + std::to_string(pos.y); }

}  // namespace OSDPrettyPlacement

#endif
