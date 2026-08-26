#include "streaming/streamingservice.h"

void StreamingService::Search(const std::string &query, SearchType type, SearchCallback callback) {
  if (type == SearchType::Songs) {
    Search(query, std::move(callback));
    return;
  }
  if (callback) {
    callback({});
  }
}

void StreamingService::GetArtists(SearchCallback callback) {
  if (callback) {
    callback({});
  }
}

void StreamingService::GetAlbums(SearchCallback callback) {
  if (callback) {
    callback({});
  }
}

void StreamingService::GetSongs(SearchCallback callback) {
  if (callback) {
    callback({});
  }
}

void StreamingService::Logout() { logged_in_ = false; }

void StreamingService::GetFavorites(FavoriteType, SearchCallback callback) {
  if (callback) {
    callback({});
  }
}

void StreamingService::AddFavorites(FavoriteType, const SongList &songs, SearchCallback callback) {
  if (callback) {
    callback(songs);
  }
}

void StreamingService::RemoveFavorites(FavoriteType, const SongList &songs, SearchCallback callback) {
  if (callback) {
    callback(songs);
  }
}
