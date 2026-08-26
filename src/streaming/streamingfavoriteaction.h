#ifndef STRAWBERRY_STREAMINGFAVORITEACTION_H
#define STRAWBERRY_STREAMINGFAVORITEACTION_H

#include "core/song.h"
#include "streaming/streamingservice.h"

namespace StreamingFavoriteAction {

inline StreamingService::FavoriteType TypeForSongs(const SongList &songs) {
  if (songs.empty()) {
    return StreamingService::FavoriteType::Songs;
  }
  bool all_albums = true;
  bool all_artists = true;
  for (const Song &song : songs) {
    if (!song.song_id().empty()) {
      return StreamingService::FavoriteType::Songs;
    }
    if (song.album_id().empty()) {
      all_albums = false;
    }
    if (song.artist_id().empty()) {
      all_artists = false;
    }
  }
  if (all_albums) {
    return StreamingService::FavoriteType::Albums;
  }
  if (all_artists) {
    return StreamingService::FavoriteType::Artists;
  }
  return StreamingService::FavoriteType::Songs;
}

}  // namespace StreamingFavoriteAction

#endif
