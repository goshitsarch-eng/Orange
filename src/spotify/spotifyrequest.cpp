#include "spotify/spotifyrequest.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

namespace SpotifyRequest {

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

std::string SearchTypeParam(Type type) {
  if (IsArtists(type)) {
    return "artist";
  }
  if (IsAlbums(type)) {
    return "album";
  }
  return "track";
}

std::string Url(const std::string &api_url, Type type, const std::string &query, int offset, int limit) {
  std::string url = api_url;
  if (IsSearch(type)) {
    url += "/search?type=" + SearchTypeParam(type) + "&q=" + StrUtils::UriEscape(query);
  } else if (type == Type::FavouriteArtists) {
    url += "/me/following?type=artist";
  } else if (type == Type::FavouriteAlbums) {
    url += "/me/albums";
  } else {
    url += "/me/tracks";
  }
  const char join = url.find('?') == std::string::npos ? '?' : '&';
  if (limit > 0) {
    url += std::string(1, join) + "limit=" + std::to_string(limit);
  }
  if (offset > 0) {
    url += std::string(url.find('?') == std::string::npos ? "?" : "&") + "offset=" + std::to_string(offset);
  }
  return url;
}

std::string ArtistAlbumsUrl(const std::string &api_url, const std::string &artist_id, int offset, int limit) {
  std::string url = api_url + "/artists/" + StrUtils::UriEscape(artist_id) + "/albums";
  if (limit > 0) {
    url += "?limit=" + std::to_string(limit);
  }
  if (offset > 0) {
    url += std::string(url.find('?') == std::string::npos ? "?" : "&") + "offset=" + std::to_string(offset);
  }
  return url;
}

std::string AlbumSongsUrl(const std::string &api_url, const std::string &album_id, int offset, int limit) {
  std::string url = api_url + "/albums/" + StrUtils::UriEscape(album_id) + "/tracks";
  if (limit > 0) {
    url += "?limit=" + std::to_string(limit);
  }
  if (offset > 0) {
    url += std::string(url.find('?') == std::string::npos ? "?" : "&") + "offset=" + std::to_string(offset);
  }
  return url;
}

SongList Parse(Type type, const std::string &json) {
  if (IsArtists(type)) {
    return JsonUtils::ParseSpotifyArtists(json);
  }
  if (IsAlbums(type)) {
    return JsonUtils::ParseSpotifyAlbums(json);
  }
  return JsonUtils::ParseSpotifyTracks(json);
}

StreamingPage::Page ParsePage(Type type, const std::string &json, int offset, int limit) {
  StreamingPage::Page page = StreamingPage::ParseMeta(json, offset, limit);
  page.songs = Parse(type, json);
  return page;
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

void GetAll(NetworkAccessManager *network, StreamingPage::UrlForOffset url_for, const std::map<std::string, std::string> &headers,
            Type type, SearchCallback callback, StreamingPage::ProgressCallback progress, StreamingPage::StillCurrent still_current,
            int limit, int max_items) {
  StreamingPage::GetAll(
      network, std::move(url_for), headers, [type](const std::string &json, int offset, int page_limit) { return ParsePage(type, json, offset, page_limit); },
      std::move(callback), std::move(progress), std::move(still_current), limit, max_items);
}

}  // namespace SpotifyRequest
