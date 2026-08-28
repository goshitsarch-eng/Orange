#ifndef STRAWBERRY_MACOSWINDOW_H
#define STRAWBERRY_MACOSWINDOW_H

namespace MacOsWindow {

// Qt mac::EnableFullScreen sets NSWindowCollectionBehaviorFullScreenPrimary (1 << 7).
inline unsigned FullScreenPrimaryMask() { return 1u << 7; }

inline bool ShouldEnableFullScreen() { return true; }

}  // namespace MacOsWindow

#endif
