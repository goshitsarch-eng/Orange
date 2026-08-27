#ifndef STRAWBERRY_OSDPRETTYLIMITS_H
#define STRAWBERRY_OSDPRETTYLIMITS_H

#include <algorithm>

namespace OSDPrettyLimits {

inline constexpr int kLabelMargin = 200;
inline constexpr int kWindowMargin = 100;
inline constexpr int kMinLabelWidth = 80;
inline constexpr int kMinWindowWidth = 160;
inline constexpr int kMinWindowHeight = 48;

inline int MaxLabelWidth(const int workarea_width) { return std::max(kMinLabelWidth, workarea_width - kLabelMargin); }

inline int MaxWindowWidth(const int workarea_width) { return std::max(kMinWindowWidth, workarea_width - kWindowMargin); }

inline int MaxWindowHeight(const int workarea_height) { return std::max(kMinWindowHeight, workarea_height - kWindowMargin); }

}  // namespace OSDPrettyLimits

#endif
