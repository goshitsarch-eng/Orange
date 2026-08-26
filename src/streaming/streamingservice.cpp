#include "streaming/streamingservice.h"

#include "streaming/streamingabort.h"
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

void StreamingService::StartArtistsProgress() {
  ArtistsUpdateStatus.Emit(StreamingProgress::ReceivingArtists());
  ArtistsProgressSetMaximum.Emit(StreamingProgress::kDefaultMaximum);
  ArtistsUpdateProgress.Emit(0);
}

void StreamingService::StartAlbumsProgress() {
  AlbumsUpdateStatus.Emit(StreamingProgress::ReceivingAlbums());
  AlbumsProgressSetMaximum.Emit(StreamingProgress::kDefaultMaximum);
  AlbumsUpdateProgress.Emit(0);
}

void StreamingService::StartSongsProgress() {
  SongsUpdateStatus.Emit(StreamingProgress::ReceivingSongs());
  SongsProgressSetMaximum.Emit(StreamingProgress::kDefaultMaximum);
  SongsUpdateProgress.Emit(0);
}

void StreamingService::CancelSearch() { ++search_gen_; }

void StreamingService::ResetArtistsRequest() { ++artists_gen_; }

void StreamingService::ResetAlbumsRequest() { ++albums_gen_; }

void StreamingService::ResetSongsRequest() { ++songs_gen_; }

int StreamingService::BeginArtistsRequest() {
  artists_gen_ = StreamingAbort::NextGeneration(artists_gen_);
  return artists_gen_;
}

int StreamingService::BeginAlbumsRequest() {
  albums_gen_ = StreamingAbort::NextGeneration(albums_gen_);
  return albums_gen_;
}

int StreamingService::BeginSongsRequest() {
  songs_gen_ = StreamingAbort::NextGeneration(songs_gen_);
  return songs_gen_;
}

int StreamingService::BeginSearchRequest() {
  search_gen_ = StreamingAbort::NextGeneration(search_gen_);
  return search_gen_;
}

bool StreamingService::ArtistsRequestCurrent(int generation) const { return StreamingAbort::IsCurrent(generation, artists_gen_); }

bool StreamingService::AlbumsRequestCurrent(int generation) const { return StreamingAbort::IsCurrent(generation, albums_gen_); }

bool StreamingService::SongsRequestCurrent(int generation) const { return StreamingAbort::IsCurrent(generation, songs_gen_); }

bool StreamingService::SearchRequestCurrent(int generation) const { return StreamingAbort::IsCurrent(generation, search_gen_); }

StreamingService::SearchCallback StreamingService::GuardArtists(SearchCallback callback) {
  const int generation = BeginArtistsRequest();
  return [this, generation, callback](const SongList &songs) {
    if (!ArtistsRequestCurrent(generation) || !callback) {
      return;
    }
    callback(songs);
  };
}

StreamingService::SearchCallback StreamingService::GuardAlbums(SearchCallback callback) {
  const int generation = BeginAlbumsRequest();
  return [this, generation, callback](const SongList &songs) {
    if (!AlbumsRequestCurrent(generation) || !callback) {
      return;
    }
    callback(songs);
  };
}

StreamingService::SearchCallback StreamingService::GuardSongs(SearchCallback callback) {
  const int generation = BeginSongsRequest();
  return [this, generation, callback](const SongList &songs) {
    if (!SongsRequestCurrent(generation) || !callback) {
      return;
    }
    callback(songs);
  };
}

StreamingService::SearchCallback StreamingService::GuardSearch(SearchCallback callback) {
  const int generation = BeginSearchRequest();
  return [this, generation, callback](const SongList &songs) {
    if (!SearchRequestCurrent(generation) || !callback) {
      return;
    }
    callback(songs);
  };
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
