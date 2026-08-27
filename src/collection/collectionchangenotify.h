#ifndef STRAWBERRY_COLLECTIONCHANGENOTIFY_H
#define STRAWBERRY_COLLECTIONCHANGENOTIFY_H

namespace CollectionChangeNotify {

inline bool ShouldEmitChanged(bool existed) { return existed; }

inline bool ShouldEmitDiscovered(bool existed) { return !existed; }

}  // namespace CollectionChangeNotify

#endif  // STRAWBERRY_COLLECTIONCHANGENOTIFY_H
