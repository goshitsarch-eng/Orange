#ifndef STRAWBERRY_STREAMINGFAVORITEACTION_H
#define STRAWBERRY_STREAMINGFAVORITEACTION_H

#include "core/song.h"
#include "streaming/streamingcollectionstore.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingservice.h"

namespace StreamingFavoriteAction {

constexpr char kFavoritesType[] = "favorites_type";

inline StreamingService::FavoriteType FromInt(int value) {
  if (value == static_cast<int>(StreamingService::FavoriteType::Artists)) {
    return StreamingService::FavoriteType::Artists;
  }
  if (value == static_cast<int>(StreamingService::FavoriteType::Albums)) {
    return StreamingService::FavoriteType::Albums;
  }
  return StreamingService::FavoriteType::Songs;
}

inline int ToInt(StreamingService::FavoriteType type) { return static_cast<int>(type); }

inline StreamingCollectionStore::List StoreList(StreamingService::FavoriteType type) {
  switch (type) {
    case StreamingService::FavoriteType::Artists:
      return StreamingCollectionStore::List::Artists;
    case StreamingService::FavoriteType::Albums:
      return StreamingCollectionStore::List::Albums;
    case StreamingService::FavoriteType::Songs:
    default:
      return StreamingCollectionStore::List::Songs;
  }
}

inline StreamingService::FavoriteType TypeFromList(StreamingCollectionStore::List list) {
  switch (list) {
    case StreamingCollectionStore::List::Artists:
      return StreamingService::FavoriteType::Artists;
    case StreamingCollectionStore::List::Albums:
      return StreamingService::FavoriteType::Albums;
    case StreamingCollectionStore::List::Songs:
    default:
      return StreamingService::FavoriteType::Songs;
  }
}

inline const char *Label(StreamingService::FavoriteType type) {
  switch (type) {
    case StreamingService::FavoriteType::Artists:
      return "Artists";
    case StreamingService::FavoriteType::Albums:
      return "Albums";
    case StreamingService::FavoriteType::Songs:
    default:
      return "Songs";
  }
}

inline const char *Receiving(StreamingService::FavoriteType type) {
  switch (type) {
    case StreamingService::FavoriteType::Artists:
      return StreamingProgress::ReceivingArtists();
    case StreamingService::FavoriteType::Albums:
      return StreamingProgress::ReceivingAlbums();
    case StreamingService::FavoriteType::Songs:
    default:
      return StreamingProgress::ReceivingSongs();
  }
}

inline const char *EmptyStatus(StreamingService::FavoriteType type, bool logged_in) {
  if (!logged_in) {
    return "Sign in in Preferences";
  }
  switch (type) {
    case StreamingService::FavoriteType::Artists:
      return "No favorite artists";
    case StreamingService::FavoriteType::Albums:
      return "No favorite albums";
    case StreamingService::FavoriteType::Songs:
    default:
      return "No favorites";
  }
}

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
