#ifndef STRAWBERRY_TIDALREQUEST_H
#define STRAWBERRY_TIDALREQUEST_H

#include "core/network.h"
#include "core/song.h"
#include "streaming/streamingservices.h"

#include <cstdint>
#include <map>
#include <string>

namespace TidalRequest {

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

std::string Resource(Type type, uint64_t user_id);
std::string Url(const std::string &api_url, Type type, const std::string &query, const std::string &country_code, uint64_t user_id = 0,
                int offset = 0, int limit = 50);
std::string ArtistAlbumsUrl(const std::string &api_url, const std::string &artist_id, const std::string &country_code, int offset = 0,
                            int limit = 50);
std::string AlbumSongsUrl(const std::string &api_url, const std::string &album_id, const std::string &country_code, int offset = 0,
                          int limit = 50);
std::string CoverUrl(const std::string &cover_id, const std::string &size = "1280x1280");

SongList Parse(Type type, const std::string &json);

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers, Type type,
         SearchCallback callback);

}  // namespace TidalRequest

#endif
