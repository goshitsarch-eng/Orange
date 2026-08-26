#ifndef STRAWBERRY_PLAYLISTSTOPAFTER_H
#define STRAWBERRY_PLAYLISTSTOPAFTER_H

#include <string>

namespace PlaylistStopAfter {

inline int ToggleRow(int current, int row) { return current == row ? -1 : row; }

inline bool IsRow(int stop_after, int row) { return stop_after >= 0 && stop_after == row; }

inline const char *TitleSuffix() { return "  [stop]"; }

inline std::string TitleText(const std::string &title, bool stop_after) { return stop_after ? title + TitleSuffix() : title; }

inline const char *CssClass() { return "playlist-stop-after"; }

}  // namespace PlaylistStopAfter

#endif  // STRAWBERRY_PLAYLISTSTOPAFTER_H
