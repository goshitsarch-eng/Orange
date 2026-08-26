#ifndef STRAWBERRY_SPOTIFYFAVORITEREQUEST_H
#define STRAWBERRY_SPOTIFYFAVORITEREQUEST_H

#include "core/network.h"
#include "core/song.h"
#include "streaming/streamingpage.h"
#include "streaming/streamingservices.h"

#include <map>
#include <string>
#include <vector>

namespace SpotifyFavoriteRequest {

using FavoriteType = StreamingService::FavoriteType;
using SearchCallback = StreamingService::SearchCallback;

std::string FavoriteText(FavoriteType type);
std::vector<std::string> IdsFromSongs(FavoriteType type, const SongList &songs);
std::string JsonIdArray(const std::vector<std::string> &ids);

std::string ListUrl(const std::string &api_url, FavoriteType type, int offset = 0, int limit = 50);

SongList Parse(FavoriteType type, const std::string &json);
std::string MutateUrl(const std::string &api_url, FavoriteType type, const std::vector<std::string> &ids);

void Get(NetworkAccessManager *network, const std::string &api_url, const std::map<std::string, std::string> &headers, FavoriteType type,
         SearchCallback callback, StreamingPage::ProgressCallback progress = {}, StreamingPage::StillCurrent still_current = {},
         StreamingPage::ErrorCallback error = {});
void Add(NetworkAccessManager *network, const std::string &api_url, const std::map<std::string, std::string> &headers, FavoriteType type,
         const SongList &songs, SearchCallback callback);
void Remove(NetworkAccessManager *network, const std::string &api_url, const std::map<std::string, std::string> &headers, FavoriteType type,
            const SongList &songs, SearchCallback callback);

}  // namespace SpotifyFavoriteRequest

#endif
