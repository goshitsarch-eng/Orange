#include "subsonic/subsonicrequest.h"

#include "utilities/jsonutils.h"

namespace SubsonicRequest {

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

bool IsArtists(Type type) { return type == Type::FavouriteArtists || type == Type::SearchArtists || type == Type::ArtistsList; }

bool IsAlbums(Type type) { return type == Type::FavouriteAlbums || type == Type::SearchAlbums || type == Type::AlbumList; }

std::string Resource(Type type) {
  switch (type) {
    case Type::FavouriteArtists:
    case Type::FavouriteAlbums:
    case Type::FavouriteSongs:
      return "getStarred2";
    case Type::SearchArtists:
    case Type::SearchAlbums:
    case Type::SearchSongs:
      return "search3";
    case Type::AlbumList:
      return "getAlbumList2";
    case Type::ArtistsList:
      return "getArtists";
    case Type::None:
      break;
  }
  return {};
}

std::map<std::string, std::string> Params(Type type, const std::string &query, int offset, int size) {
  std::map<std::string, std::string> params;
  if (IsSearch(type)) {
    params["query"] = query;
    params["artistCount"] = type == Type::SearchArtists ? std::to_string(size) : "0";
    params["albumCount"] = type == Type::SearchAlbums ? std::to_string(size) : "0";
    params["songCount"] = type == Type::SearchSongs ? std::to_string(size) : "0";
  } else if (type == Type::AlbumList) {
    params["type"] = "alphabeticalByName";
    if (size > 0) {
      params["size"] = std::to_string(size);
    }
    if (offset > 0) {
      params["offset"] = std::to_string(offset);
    }
  }
  return params;
}

std::map<std::string, std::string> AlbumSongsParams(const std::string &album_id) { return {{"id", album_id}}; }

SongList Parse(Type type, const std::string &json) {
  if (IsArtists(type)) {
    return JsonUtils::ParseSubsonicArtists(json);
  }
  if (IsAlbums(type)) {
    return JsonUtils::ParseSubsonicAlbums(json);
  }
  return JsonUtils::ParseSubsonicSongs(json);
}

void Get(NetworkAccessManager *network, const std::string &url, Type type, SearchCallback callback) {
  if (!network || url.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  network->Get(url, [type, callback](const NetworkAccessManager::Response &response) {
    if (!callback) {
      return;
    }
    callback(response.ok() ? Parse(type, response.body) : SongList{});
  });
}

}  // namespace SubsonicRequest
