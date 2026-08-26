#ifndef STRAWBERRY_TRAYPOPUP_H
#define STRAWBERRY_TRAYPOPUP_H

#include "osd/osdprettyfade.h"

namespace TrayPopup {

inline bool ShowArt(bool show_art, bool has_bytes) { return show_art && has_bytes; }

inline double FadeOpacity(int elapsed_ms, int duration_ms, bool fading_in) {
  return OSDPrettyFade::OpacityAt(elapsed_ms, duration_ms, fading_in);
}

inline int FadeDurationMs() { return OSDPrettyFade::kDurationMs; }

inline int FadeTickMs() { return OSDPrettyFade::kTickMs; }

}  // namespace TrayPopup

#endif  // STRAWBERRY_TRAYPOPUP_H
