#ifndef STRAWBERRY_PLAYLISTQUEUEDEQUEUE_H
#define STRAWBERRY_PLAYLISTQUEUEDEQUEUE_H

#include "queue/queue.h"

namespace PlaylistQueueDequeue {

inline bool ShouldDequeue(int playlist_id, int row, const Queue &queue) {
  return playlist_id >= 0 && row >= 0 && queue.PositionForPlaylistRow(playlist_id, row) == 1;
}

}  // namespace PlaylistQueueDequeue

#endif  // STRAWBERRY_PLAYLISTQUEUEDEQUEUE_H
