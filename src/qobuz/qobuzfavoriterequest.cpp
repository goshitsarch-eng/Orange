#include "qobuz/qobuzfavoriterequest.h"

#include "qobuz/qobuzrequest.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <algorithm>

namespace QobuzFavoriteRequest {

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

std::string FavoriteMethod(FavoriteType type) {
  switch (type) {
    case FavoriteType::Artists:
      return "artist_ids";
    case FavoriteType::Albums:
      return "album_ids";
    case FavoriteType::Songs:
      return "track_ids";
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

std::string AuthQuery(const std::string &app_id, const std::string &user_auth_token) {
  std::string query;
  if (!app_id.empty()) {
    query += "&app_id=" + StrUtils::UriEscape(app_id);
  }
  if (!user_auth_token.empty()) {
    query += "&user_auth_token=" + StrUtils::UriEscape(user_auth_token);
  }
  return query;
}

std::string ListUrl(const std::string &api_url, FavoriteType type, const std::string &app_id, const std::string &user_auth_token, int offset,
                    int limit) {
  std::string url = api_url + "/favorite/getUserFavorites?type=" + FavoriteText(type) + AuthQuery(app_id, user_auth_token);
  if (limit > 0) {
    url += "&limit=" + std::to_string(limit);
  }
  if (offset > 0) {
    url += "&offset=" + std::to_string(offset);
  }
  return url;
}

std::string CreateUrl(const std::string &api_url, FavoriteType type, const std::vector<std::string> &ids, const std::string &app_id,
                      const std::string &user_auth_token) {
  return api_url + "/favorite/create?" + FavoriteMethod(type) + "=" + StrUtils::UriEscape(JoinedIds(ids)) + AuthQuery(app_id, user_auth_token);
}

std::string DeleteUrl(const std::string &api_url, FavoriteType type, const std::vector<std::string> &ids, const std::string &app_id,
                      const std::string &user_auth_token) {
  return api_url + "/favorite/delete?" + FavoriteMethod(type) + "=" + StrUtils::UriEscape(JoinedIds(ids)) + AuthQuery(app_id, user_auth_token);
}

SongList Parse(FavoriteType type, const std::string &json) { return QobuzRequest::Parse(QobuzRequest::FromFavoriteType(type), json); }

void Get(NetworkAccessManager *network, const std::string &api_url, const std::string &app_id, const std::string &user_auth_token,
         const std::map<std::string, std::string> &headers, FavoriteType type, SearchCallback callback,
         StreamingPage::ProgressCallback progress, StreamingPage::StillCurrent still_current) {
  QobuzRequest::GetAll(
      network,
      [api_url, app_id, user_auth_token, type](int offset, int limit) { return ListUrl(api_url, type, app_id, user_auth_token, offset, limit); },
      headers, QobuzRequest::FromFavoriteType(type), std::move(callback), std::move(progress), std::move(still_current));
}

void Add(NetworkAccessManager *network, const std::string &api_url, const std::string &app_id, const std::string &user_auth_token,
         const std::map<std::string, std::string> &headers, FavoriteType type, const SongList &songs, SearchCallback callback) {
  const std::vector<std::string> ids = IdsFromSongs(type, songs);
  if (!network || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Get(CreateUrl(api_url, type, ids, app_id, user_auth_token),
               [callback, songs](const NetworkAccessManager::Response &response) {
                 if (callback) {
                   callback(response.ok() ? songs : SongList{});
                 }
               },
               headers);
}

void Remove(NetworkAccessManager *network, const std::string &api_url, const std::string &app_id, const std::string &user_auth_token,
            const std::map<std::string, std::string> &headers, FavoriteType type, const SongList &songs, SearchCallback callback) {
  const std::vector<std::string> ids = IdsFromSongs(type, songs);
  if (!network || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Get(DeleteUrl(api_url, type, ids, app_id, user_auth_token),
               [callback, songs](const NetworkAccessManager::Response &response) {
                 if (callback) {
                   callback(response.ok() ? songs : SongList{});
                 }
               },
               headers);
}

}  // namespace QobuzFavoriteRequest
