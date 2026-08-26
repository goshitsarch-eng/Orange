#include "tidal/tidalfavoriterequest.h"

#include "tidal/tidalrequest.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <algorithm>

namespace TidalFavoriteRequest {

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
      return "artistIds";
    case FavoriteType::Albums:
      return "albumIds";
    case FavoriteType::Songs:
      return "trackIds";
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

std::string ListUrl(const std::string &api_url, uint64_t user_id, FavoriteType type, const std::string &country_code, int offset, int limit) {
  std::string url = api_url + "/users/" + std::to_string(user_id) + "/favorites/" + FavoriteText(type) +
                    "?countryCode=" + StrUtils::UriEscape(country_code);
  if (limit > 0) {
    url += "&limit=" + std::to_string(limit);
  }
  if (offset > 0) {
    url += "&offset=" + std::to_string(offset);
  }
  return url;
}

std::string AddUrl(const std::string &api_url, uint64_t user_id, FavoriteType type) {
  return api_url + "/users/" + std::to_string(user_id) + "/favorites/" + FavoriteText(type);
}

std::string AddFormBody(FavoriteType type, const std::string &country_code, const std::vector<std::string> &ids) {
  std::string joined;
  for (size_t i = 0; i < ids.size(); ++i) {
    if (i > 0) {
      joined += ",";
    }
    joined += ids[i];
  }
  return "countryCode=" + StrUtils::UriEscape(country_code) + "&" + FavoriteMethod(type) + "=" + StrUtils::UriEscape(joined);
}

std::string RemoveUrl(const std::string &api_url, uint64_t user_id, FavoriteType type, const std::string &id, const std::string &country_code) {
  return api_url + "/users/" + std::to_string(user_id) + "/favorites/" + FavoriteText(type) + "/" + StrUtils::UriEscape(id) +
         "?countryCode=" + StrUtils::UriEscape(country_code);
}

SongList Parse(FavoriteType type, const std::string &json) { return TidalRequest::Parse(TidalRequest::FromFavoriteType(type), json); }

void Get(NetworkAccessManager *network, const std::string &api_url, uint64_t user_id, const std::string &country_code,
         const std::map<std::string, std::string> &headers, FavoriteType type, SearchCallback callback,
         StreamingPage::ProgressCallback progress, StreamingPage::StillCurrent still_current, StreamingPage::ErrorCallback error) {
  TidalRequest::GetAll(
      network,
      [api_url, user_id, country_code, type](int offset, int limit) { return ListUrl(api_url, user_id, type, country_code, offset, limit); },
      headers, TidalRequest::FromFavoriteType(type), std::move(callback), std::move(progress), std::move(still_current),
      StreamingPage::kDefaultLimit, 0, std::move(error));
}

void Add(NetworkAccessManager *network, const std::string &api_url, uint64_t user_id, const std::string &country_code,
         const std::map<std::string, std::string> &headers, FavoriteType type, const SongList &songs, SearchCallback callback) {
  const std::vector<std::string> ids = IdsFromSongs(type, songs);
  if (!network || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Post(AddUrl(api_url, user_id, type), AddFormBody(type, country_code, ids),
                [callback, songs](const NetworkAccessManager::Response &response) {
                  if (callback) {
                    callback(response.ok() ? songs : SongList{});
                  }
                },
                "application/x-www-form-urlencoded", headers);
}

void Remove(NetworkAccessManager *network, const std::string &api_url, uint64_t user_id, const std::string &country_code,
            const std::map<std::string, std::string> &headers, FavoriteType type, const SongList &songs, SearchCallback callback) {
  const std::vector<std::string> ids = IdsFromSongs(type, songs);
  if (!network || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Delete(RemoveUrl(api_url, user_id, type, ids.front(), country_code),
                  [callback, songs](const NetworkAccessManager::Response &response) {
                    if (callback) {
                      callback(response.ok() ? songs : SongList{});
                    }
                  },
                  headers);
}

}  // namespace TidalFavoriteRequest
