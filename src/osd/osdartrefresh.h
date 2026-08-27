#ifndef STRAWBERRY_OSDARTREFRESH_H
#define STRAWBERRY_OSDARTREFRESH_H

#include "core/song.h"

#include <vector>

namespace OsdArtRefresh {

inline bool ShouldDeferUntilCover(bool show_art, bool art_empty) { return show_art && art_empty; }

inline bool MatchesPlaying(const Song &playing, const Song &loaded) {
  return playing.url() == loaded.url() && playing.beginning_nanosec() == loaded.beginning_nanosec();
}

inline bool ShouldRefresh(bool show_art, bool previous_art_empty, bool new_art_present) {
  return show_art && previous_art_empty && new_art_present;
}

}  // namespace OsdArtRefresh

#endif  // STRAWBERRY_OSDARTREFRESH_H
