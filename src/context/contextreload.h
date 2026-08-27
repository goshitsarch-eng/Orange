#ifndef STRAWBERRY_CONTEXTRELOAD_H
#define STRAWBERRY_CONTEXTRELOAD_H

namespace ContextReload {

// Qt ContextView::ReloadSettings calls NoSong() or SetSong() so title/summary/fonts refresh after Preferences.

inline bool ShouldRefreshDisplayOnReload() { return true; }

inline bool ShouldRefreshIdle(bool idle) { return idle; }

inline bool ShouldRefreshPlaying(bool idle) { return !idle; }

}  // namespace ContextReload

#endif
