#include "streaming/streamingservice.h"

#include "streaming/streamingprogress.h"

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

void StreamingService::GetArtistAlbums(const Song &, SearchCallback callback) {
  if (callback) {
    callback({});
  }
}

void StreamingService::GetAlbumSongs(const Song &, SearchCallback callback) {
  if (callback) {
    callback({});
  }
}

void StreamingService::Logout() {
  logged_in_ = false;
  NotifyAuthenticationChanged();
}

void StreamingService::NotifyAuthenticationChanged() { AuthenticationChanged.Emit(); }

int StreamingService::StartSearchProgress() {
  ++last_search_id_;
  SearchUpdateStatus.Emit(last_search_id_, StreamingProgress::Searching());
  SearchProgressSetMaximum.Emit(last_search_id_, StreamingProgress::kDefaultMaximum);
  SearchUpdateProgress.Emit(last_search_id_, 0);
  return last_search_id_;
}

void StreamingService::NotifyAuthenticationFailed(const std::string &error) { AuthenticationFailed.Emit(error); }

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
