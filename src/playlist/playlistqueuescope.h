#ifndef STRAWBERRY_PLAYLISTQUEUESCOPE_H
#define STRAWBERRY_PLAYLISTQUEUESCOPE_H

#include "playlist/playlist.h"
#include "queue/queue.h"

namespace PlaylistQueueScope {

inline Queue *For(Playlist *playlist, Queue *fallback = nullptr) { return playlist ? playlist->queue() : fallback; }

inline const Queue *For(const Playlist *playlist, const Queue *fallback = nullptr) { return playlist ? playlist->queue() : fallback; }

}  // namespace PlaylistQueueScope

#endif  // STRAWBERRY_PLAYLISTQUEUESCOPE_H
