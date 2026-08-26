#include "qobuz/qobuzrequest.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

namespace QobuzRequest {

Type FromSearchType(SearchType type) {
  switch (type) {
    case SearchType::Artists:
      return Type::SearchArtists;
    case SearchType::Albums:
      return Type::SearchAlbums;
    case SearchType::Songs:
      return Type::SearchSongs;
  }
  return Type::None;
}

Type FromFavoriteType(StreamingService::FavoriteType type) {
  switch (type) {
    case StreamingService::FavoriteType::Artists:
      return Type::FavouriteArtists;
    case StreamingService::FavoriteType::Albums:
      return Type::FavouriteAlbums;
    case StreamingService::FavoriteType::Songs:
      return Type::FavouriteSongs;
  }
  return Type::None;
}

bool IsSearch(Type type) {
  return type == Type::SearchArtists || type == Type::SearchAlbums || type == Type::SearchSongs;
}

bool IsArtists(Type type) { return type == Type::FavouriteArtists || type == Type::SearchArtists; }

bool IsAlbums(Type type) { return type == Type::FavouriteAlbums || type == Type::SearchAlbums; }

std::string Resource(Type type) {
  switch (type) {
    case Type::FavouriteArtists:
    case Type::FavouriteAlbums:
    case Type::FavouriteSongs:
      return "favorite/getUserFavorites";
    case Type::SearchArtists:
      return "artist/search";
    case Type::SearchAlbums:
      return "album/search";
    case Type::SearchSongs:
      return "track/search";
    case Type::StreamURL:
      return "track/getFileUrl";
    case Type::None:
      break;
  }
  return {};
}

namespace {

std::string AppendAuth(std::string url, const std::string &app_id, const std::string &user_auth_token, int offset, int limit) {
  if (!app_id.empty()) {
    url += (url.find('?') == std::string::npos ? "?" : "&") + std::string("app_id=") + StrUtils::UriEscape(app_id);
  }
  if (!user_auth_token.empty()) {
    url += (url.find('?') == std::string::npos ? "?" : "&") + std::string("user_auth_token=") + StrUtils::UriEscape(user_auth_token);
  }
  if (limit > 0) {
    url += (url.find('?') == std::string::npos ? "?" : "&") + std::string("limit=") + std::to_string(limit);
  }
  if (offset > 0) {
    url += (url.find('?') == std::string::npos ? "?" : "&") + std::string("offset=") + std::to_string(offset);
  }
  return url;
}

}  // namespace

std::string Url(const std::string &api_url, Type type, const std::string &query, const std::string &app_id,
                const std::string &user_auth_token, int offset, int limit) {
  std::string url = api_url + "/" + Resource(type);
  if (IsSearch(type)) {
    url += "?query=" + StrUtils::UriEscape(query);
  } else if (type == Type::FavouriteArtists) {
    url += "?type=artists";
  } else if (type == Type::FavouriteAlbums) {
    url += "?type=albums";
  } else if (type == Type::FavouriteSongs) {
    url += "?type=tracks";
  }
  return AppendAuth(url, app_id, user_auth_token, offset, limit);
}

std::string ArtistAlbumsUrl(const std::string &api_url, const std::string &artist_id, const std::string &app_id,
                            const std::string &user_auth_token) {
  return AppendAuth(api_url + "/artist/get?artist_id=" + StrUtils::UriEscape(artist_id) + "&extra=albums", app_id, user_auth_token, 0, 0);
}

std::string AlbumSongsUrl(const std::string &api_url, const std::string &album_id, const std::string &app_id,
                          const std::string &user_auth_token) {
  return AppendAuth(api_url + "/album/get?album_id=" + StrUtils::UriEscape(album_id), app_id, user_auth_token, 0, 0);
}

SongList Parse(Type type, const std::string &json) {
  if (IsArtists(type)) {
    return JsonUtils::ParseQobuzArtists(json);
  }
  if (IsAlbums(type)) {
    return JsonUtils::ParseQobuzAlbums(json);
  }
  return JsonUtils::ParseQobuzTracks(json);
}

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers, Type type,
         SearchCallback callback) {
  if (!network || url.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Get(
      url,
      [type, callback](const NetworkAccessManager::Response &response) {
        if (!callback) {
          return;
        }
        callback(response.ok() ? Parse(type, response.body) : SongList{});
      },
      headers);
}

}  // namespace QobuzRequest
