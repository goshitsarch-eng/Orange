#ifndef STRAWBERRY_PLAYLISTCOLLECTIONSYNC_H
#define STRAWBERRY_PLAYLISTCOLLECTIONSYNC_H

#include "core/song.h"
#include "playlist/playlist.h"

#include <vector>

namespace PlaylistCollectionSync {

inline bool SameCollectionRow(const Song &existing, const Song &incoming) {
  if (incoming.id() <= 0 || existing.id() != incoming.id()) {
    return false;
  }
  if (incoming.directory_id() > 0 && existing.directory_id() > 0 && incoming.directory_id() != existing.directory_id()) {
    return false;
  }
  return true;
}

inline int PatchPlaylist(Playlist *playlist, const SongList &songs) {
  if (!playlist) {
    return 0;
  }
  int patched = 0;
  for (const Song &song : songs) {
    if (playlist->PatchSongById(song)) {
      ++patched;
    }
  }
  return patched;
}

inline int PatchAll(const std::vector<Playlist *> &playlists, const SongList &songs) {
  int patched = 0;
  for (Playlist *playlist : playlists) {
    patched += PatchPlaylist(playlist, songs);
  }
  return patched;
}

}  // namespace PlaylistCollectionSync

#endif  // STRAWBERRY_PLAYLISTCOLLECTIONSYNC_H
