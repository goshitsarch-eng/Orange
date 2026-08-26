#include "spotify/spotifyfavoriterequest.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <algorithm>

namespace SpotifyFavoriteRequest {

std::string FavoriteText(FavoriteType type) {
  switch (type) {
    case FavoriteType::Artists:
      return "artists";
    case FavoriteType::Albums:
      return "albums";
    case FavoriteType::Songs:
      return "tracks";
  }
  return {};
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

std::string JsonIdArray(const std::vector<std::string> &ids) {
  std::string json = "[";
  for (size_t i = 0; i < ids.size(); ++i) {
    if (i > 0) {
      json += ",";
    }
    json += "\"" + StrUtils::JsonEscape(ids[i]) + "\"";
  }
  json += "]";
  return json;
}

std::string JoinedIds(const std::vector<std::string> &ids) {
  std::string joined;
  for (size_t i = 0; i < ids.size(); ++i) {
    if (i > 0) {
      joined += ",";
    }
    joined += ids[i];
  }
  return joined;
}

std::string ListUrl(const std::string &api_url, FavoriteType type, int limit) {
  if (type == FavoriteType::Artists) {
    return api_url + "/me/following?type=artist&limit=" + std::to_string(limit);
  }
  return api_url + "/me/" + FavoriteText(type) + "?limit=" + std::to_string(limit);
}

std::string MutateUrl(const std::string &api_url, FavoriteType type, const std::vector<std::string> &ids) {
  if (type == FavoriteType::Artists) {
    return api_url + "/me/following?type=artist&ids=" + StrUtils::UriEscape(JoinedIds(ids));
  }
  return api_url + "/me/" + FavoriteText(type) + "?ids=" + StrUtils::UriEscape(JoinedIds(ids));
}

void Get(NetworkAccessManager *network, const std::string &api_url, const std::map<std::string, std::string> &headers, FavoriteType type,
         SearchCallback callback) {
  if (!network || !callback) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Get(ListUrl(api_url, type), [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({});
      return;
    }
    callback(JsonUtils::ParseSpotifyTracks(response.body));
  }, headers);
}

void Add(NetworkAccessManager *network, const std::string &api_url, const std::map<std::string, std::string> &headers, FavoriteType type,
         const SongList &songs, SearchCallback callback) {
  const std::vector<std::string> ids = IdsFromSongs(type, songs);
  if (!network || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  const std::string body = type == FavoriteType::Artists ? std::string() : JsonIdArray(ids);
  network->Put(MutateUrl(api_url, type, ids), body,
               [callback, songs](const NetworkAccessManager::Response &response) {
                 if (callback) {
                   callback(response.ok() ? songs : SongList{});
                 }
               },
               "application/x-www-form-urlencoded", headers);
}

void Remove(NetworkAccessManager *network, const std::string &api_url, const std::map<std::string, std::string> &headers, FavoriteType type,
            const SongList &songs, SearchCallback callback) {
  const std::vector<std::string> ids = IdsFromSongs(type, songs);
  if (!network || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Delete(MutateUrl(api_url, type, ids),
                  [callback, songs](const NetworkAccessManager::Response &response) {
                    if (callback) {
                      callback(response.ok() ? songs : SongList{});
                    }
                  },
                  headers);
}

}  // namespace SpotifyFavoriteRequest
