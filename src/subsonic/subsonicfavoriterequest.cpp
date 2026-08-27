#include "subsonic/subsonicfavoriterequest.h"

#include "streaming/streamingabort.h"
#include "utilities/jsonutils.h"

#include <algorithm>
#include <memory>

namespace SubsonicFavoriteRequest {

std::string IdParam(FavoriteType type) {
  switch (type) {
    case FavoriteType::Artists:
      return "artistId";
    case FavoriteType::Albums:
      return "albumId";
    case FavoriteType::Songs:
      return "id";
  }
  return "id";
}

std::vector<std::string> IdsFromSongs(FavoriteType type, const SongList &songs) {
  std::vector<std::string> ids;
  for (const Song &song : songs) {
    std::string id;
    switch (type) {
      case FavoriteType::Artists:
        id = song.artist_id();
        break;
      case FavoriteType::Albums:
        id = song.album_id();
        break;
      case FavoriteType::Songs:
        id = song.song_id();
        break;
    }
    if (id.empty()) {
      continue;
    }
    if (std::find(ids.begin(), ids.end(), id) == ids.end()) {
      ids.push_back(id);
    }
  }
  return ids;
}

std::string StarResource(bool remove) { return remove ? "unstar" : "star"; }

std::map<std::string, std::string> StarParams(FavoriteType type, const std::string &id) { return {{IdParam(type), id}}; }

void Get(NetworkAccessManager *network, const std::string &list_url, SearchCallback callback, StreamingPage::ErrorCallback error) {
  if (!network || list_url.empty()) {
    if (error) {
      error(StreamingAbort::HttpError(0, {}));
    }
    if (callback) {
      callback({});
    }
    return;
  }
  network->Get(list_url, [callback, error](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      if (error) {
        error(StreamingAbort::HttpError(response.status, response.error));
      }
      if (callback) {
        callback({});
      }
      return;
    }
    if (callback) {
      callback(JsonUtils::ParseSubsonicSongs(response.body));
    }
  });
}

void Mutate(NetworkAccessManager *network, const std::string &url, const SongList &songs, SearchCallback callback) {
  MutateMany(network, url.empty() ? std::vector<std::string>{} : std::vector<std::string>{url}, songs, std::move(callback));
}

void MutateMany(NetworkAccessManager *network, const std::vector<std::string> &urls, const SongList &songs, SearchCallback callback) {
  if (!network || urls.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  auto remaining = std::make_shared<int>(static_cast<int>(urls.size()));
  auto ok = std::make_shared<bool>(true);
  for (const std::string &url : urls) {
    network->Get(url, [callback, songs, remaining, ok](const NetworkAccessManager::Response &response) {
      if (!response.ok()) {
        *ok = false;
      }
      if (--*remaining == 0 && callback) {
        callback(*ok ? songs : SongList{});
      }
    });
  }
}

}  // namespace SubsonicFavoriteRequest
