#ifndef STRAWBERRY_DIALOGSONGNAV_H
#define STRAWBERRY_DIALOGSONGNAV_H

namespace DialogSongNav {

// Qt EditTagDialog / TrackSelectionDialog QShortcut Back, Forward, PageUp, PageDown.
constexpr unsigned kPageUp = 0xff55;
constexpr unsigned kPageDown = 0xff56;
constexpr unsigned kKPPageUp = 0xff9a;
constexpr unsigned kKPPageDown = 0xff9b;
constexpr unsigned kLeft = 0xff51;
constexpr unsigned kRight = 0xff53;
constexpr unsigned kAltMask = 1u << 3;
constexpr unsigned kXF86Back = 0x1008ff26;
constexpr unsigned kXF86Forward = 0x1008ff27;

inline bool IsPrevious(unsigned keyval, unsigned state) {
  if (keyval == kPageUp || keyval == kKPPageUp || keyval == kXF86Back) {
    return true;
  }
  return keyval == kLeft && (state & kAltMask) != 0;
}

inline bool IsNext(unsigned keyval, unsigned state) {
  if (keyval == kPageDown || keyval == kKPPageDown || keyval == kXF86Forward) {
    return true;
  }
  return keyval == kRight && (state & kAltMask) != 0;
}

inline int Delta(unsigned keyval, unsigned state) {
  if (IsPrevious(keyval, state)) {
    return -1;
  }
  if (IsNext(keyval, state)) {
    return 1;
  }
  return 0;
}

}  // namespace DialogSongNav

#endif
