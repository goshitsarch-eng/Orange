#ifndef STRAWBERRY_TRAYICONMASK_H
#define STRAWBERRY_TRAYICONMASK_H

#include "systemtrayicon/trayiconcomposite.h"

#include <vector>

namespace TrayIconMask {

using Point = TrayIconComposite::Point;

inline std::vector<Point> ProgressMask(int width, int height, int percentage) {
  std::vector<Point> points;
  points.push_back({0, 0});
  const Point end = TrayIconComposite::CoverLineEnd(width, height, percentage);
  points.push_back(end);
  if (TrayIconComposite::CoverIncludesBottomRight(percentage)) {
    points.push_back({width, height});
  }
  points.push_back({width, 0});
  points.push_back({0, 0});
  return points;
}

}  // namespace TrayIconMask

#endif  // STRAWBERRY_TRAYICONMASK_H
