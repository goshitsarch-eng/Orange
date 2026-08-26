#ifndef STRAWBERRY_QOBUZREQUEST_H
#define STRAWBERRY_QOBUZREQUEST_H

#include "core/network.h"
#include "core/song.h"
#include "streaming/streamingservices.h"

#include <map>
#include <string>

namespace QobuzRequest {

using SearchCallback = StreamingService::SearchCallback;
using SearchType = StreamingService::SearchType;

enum class Type {
  None,
  FavouriteArtists,
  FavouriteAlbums,
  FavouriteSongs,
  SearchArtists,
  SearchAlbums,
  SearchSongs,
  StreamURL
};

Type FromSearchType(SearchType type);
Type FromFavoriteType(StreamingService::FavoriteType type);
bool IsSearch(Type type);
bool IsArtists(Type type);
bool IsAlbums(Type type);

std::string Resource(Type type);
std::string Url(const std::string &api_url, Type type, const std::string &query, const std::string &app_id,
                const std::string &user_auth_token, int offset = 0, int limit = 50);
std::string ArtistAlbumsUrl(const std::string &api_url, const std::string &artist_id, const std::string &app_id,
                            const std::string &user_auth_token);
std::string AlbumSongsUrl(const std::string &api_url, const std::string &album_id, const std::string &app_id,
                          const std::string &user_auth_token);

SongList Parse(Type type, const std::string &json);

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers, Type type,
         SearchCallback callback);

}  // namespace QobuzRequest

#endif
