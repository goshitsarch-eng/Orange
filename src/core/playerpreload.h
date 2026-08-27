#ifndef STRAWBERRY_PLAYERPRELOAD_H
#define STRAWBERRY_PLAYERPRELOAD_H

namespace PlayerPreload {

inline bool ShouldPreload(bool stop_after, bool has_next) { return !stop_after && has_next; }

inline bool CanPreload(bool stop_after, bool has_next, bool current_is_module) {
  return ShouldPreload(stop_after, has_next) && !current_is_module;
}

inline bool ShouldAdvanceOnAboutToEnd(bool autocrossfade, bool same_album, bool no_crossfade_same_album) {
  if (!autocrossfade) {
    return false;
  }
  if (same_album && no_crossfade_same_album) {
    return false;
  }
  return true;
}

}  // namespace PlayerPreload

#endif  // STRAWBERRY_PLAYERPRELOAD_H
