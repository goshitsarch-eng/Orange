#ifndef STRAWBERRY_SUBSONICREQUEST_H
#define STRAWBERRY_SUBSONICREQUEST_H

#include "core/network.h"
#include "core/song.h"
#include "streaming/streamingpage.h"
#include "streaming/streamingservices.h"

#include <map>
#include <string>

namespace SubsonicRequest {

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
  AlbumList,
  ArtistsList,
  ArtistAlbums,
  AlbumSongs
};

Type FromSearchType(SearchType type);
Type FromFavoriteType(StreamingService::FavoriteType type);
bool IsSearch(Type type);
bool IsArtists(Type type);
bool IsAlbums(Type type);

std::string Resource(Type type);
std::map<std::string, std::string> Params(Type type, const std::string &query, int offset = 0, int size = 50);
std::map<std::string, std::string> AlbumSongsParams(const std::string &album_id);
std::map<std::string, std::string> ArtistAlbumsParams(const std::string &artist_id);

SongList Parse(Type type, const std::string &json);

StreamingPage::Page ParsePage(Type type, const std::string &json, int offset, int limit);

void Get(NetworkAccessManager *network, const std::string &url, Type type, SearchCallback callback,
         StreamingPage::ErrorCallback error = {});
void GetAll(NetworkAccessManager *network, StreamingPage::UrlForOffset url_for, Type type, SearchCallback callback,
            StreamingPage::ProgressCallback progress = {}, StreamingPage::StillCurrent still_current = {},
            int limit = StreamingPage::kDefaultLimit, int max_items = 0, StreamingPage::ErrorCallback error = {});

}  // namespace SubsonicRequest

#endif
