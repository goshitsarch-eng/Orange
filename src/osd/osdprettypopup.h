#ifndef STRAWBERRY_OSDPRETTYPOPUP_H
#define STRAWBERRY_OSDPRETTYPOPUP_H

#include <algorithm>
#include <string>

namespace OSDPrettyPopup {

inline constexpr double kHoverOpacity = 0.25;
inline constexpr int kSnapProximity = 20;
inline const char *kSnapClass = "osd-pretty-snapped";

inline bool DragEnabled(bool draggable) { return draggable; }
inline bool ClickDismisses(bool popup) { return popup; }
inline bool HoverDims(bool popup) { return popup; }
inline bool HideArtWhenEmpty(bool show_art, bool has_image) { return !show_art || !has_image; }

inline bool ShouldHideOnRepeat(bool visible, bool popup, bool toggle_mode) { return visible && popup && toggle_mode; }
inline bool ShouldRestartTimeout(bool visible, bool popup, bool toggle_mode) { return visible && popup && !toggle_mode; }

inline bool IsSnappedToCenter(int x, int center, int proximity = kSnapProximity) {
  return x > center - proximity && x < center + proximity;
}

inline std::string ChromeCss(const std::string &background, const std::string &foreground, double opacity, const std::string &font_css) {
  const double alpha = std::clamp(opacity, 0.2, 1.0);
  return ".osd-pretty { background-color: alpha(" + background + ", " + std::to_string(alpha) + "); color: " + foreground +
         "; font: " + font_css +
         "; border-radius: 10px; border: 1px solid alpha(#ffffff, 0.18); "
         "box-shadow: 0 8px 24px alpha(#000000, 0.45), inset 0 1px 0 alpha(#ffffff, 0.12); } "
         ".osd-pretty.osd-pretty-snapped { outline: 2px solid alpha(#ffffff, 0.55); }";
}

}  // namespace OSDPrettyPopup

#endif
