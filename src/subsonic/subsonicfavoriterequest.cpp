#include "subsonic/subsonicfavoriterequest.h"

#include "utilities/jsonutils.h"

#include <algorithm>

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

void Get(NetworkAccessManager *network, const std::string &list_url, SearchCallback callback) {
  if (!network || !callback) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Get(list_url, [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({});
      return;
    }
    callback(JsonUtils::ParseSubsonicSongs(response.body));
  });
}

void Mutate(NetworkAccessManager *network, const std::string &url, const SongList &songs, SearchCallback callback) {
  if (!network) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Get(url, [callback, songs](const NetworkAccessManager::Response &response) {
    if (callback) {
      callback(response.ok() ? songs : SongList{});
    }
  });
}

}  // namespace SubsonicFavoriteRequest
