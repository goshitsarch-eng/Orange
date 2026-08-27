#ifndef STRAWBERRY_PLAYLISTTABNAV_H
#define STRAWBERRY_PLAYLISTTABNAV_H

namespace PlaylistTabNavigation {

inline int Wrap(int index, int count) {
  if (count <= 0) {
    return -1;
  }
  int wrapped = index % count;
  if (wrapped < 0) {
    wrapped += count;
  }
  return wrapped;
}

inline int NextIndex(int current, int count) {
  if (current < 0 || current >= count) {
    return count > 0 ? 0 : -1;
  }
  return Wrap(current + 1, count);
}

inline int PreviousIndex(int current, int count) {
  if (current < 0 || current >= count) {
    return count > 0 ? count - 1 : -1;
  }
  return Wrap(current - 1, count);
}

inline int LastIndex(int count) { return count > 0 ? count - 1 : -1; }

inline int ActiveIndex(int active, int count) {
  if (active < 0 || active >= count) {
    return -1;
  }
  return active;
}

}  // namespace PlaylistTabNavigation

#endif
