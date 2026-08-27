#ifndef STRAWBERRY_PLAYLISTTABBARVISIBILITY_H
#define STRAWBERRY_PLAYLISTTABBARVISIBILITY_H

#include <algorithm>

namespace PlaylistTabBarVisibility {

inline constexpr int kAnimationMs = 500;

inline bool ShouldShow(int playlist_count) { return playlist_count > 1; }

inline int HeightAt(int elapsed_ms, int natural_height, bool showing) {
  if (natural_height < 0) {
    natural_height = 0;
  }
  const int clamped = std::clamp(elapsed_ms, 0, kAnimationMs);
  const int grown = natural_height * clamped / kAnimationMs;
  return showing ? grown : natural_height - grown;
}

}  // namespace PlaylistTabBarVisibility

#endif  // STRAWBERRY_PLAYLISTTABBARVISIBILITY_H
