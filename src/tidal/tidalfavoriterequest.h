#ifndef STRAWBERRY_TIDALFAVORITEREQUEST_H
#define STRAWBERRY_TIDALFAVORITEREQUEST_H

#include "core/network.h"
#include "core/song.h"
#include "streaming/streamingpage.h"
#include "streaming/streamingservices.h"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace TidalFavoriteRequest {

using FavoriteType = StreamingService::FavoriteType;
using SearchCallback = StreamingService::SearchCallback;

std::string FavoriteText(FavoriteType type);
std::string FavoriteMethod(FavoriteType type);
std::vector<std::string> IdsFromSongs(FavoriteType type, const SongList &songs);

std::string ListUrl(const std::string &api_url, uint64_t user_id, FavoriteType type, const std::string &country_code, int offset = 0,
                    int limit = 50);
std::string AddUrl(const std::string &api_url, uint64_t user_id, FavoriteType type);
std::string AddFormBody(FavoriteType type, const std::string &country_code, const std::vector<std::string> &ids);
std::string RemoveUrl(const std::string &api_url, uint64_t user_id, FavoriteType type, const std::string &id, const std::string &country_code);

SongList Parse(FavoriteType type, const std::string &json);

void Get(NetworkAccessManager *network, const std::string &api_url, uint64_t user_id, const std::string &country_code,
         const std::map<std::string, std::string> &headers, FavoriteType type, SearchCallback callback,
         StreamingPage::ProgressCallback progress = {}, StreamingPage::StillCurrent still_current = {},
         StreamingPage::ErrorCallback error = {});
void Add(NetworkAccessManager *network, const std::string &api_url, uint64_t user_id, const std::string &country_code,
         const std::map<std::string, std::string> &headers, FavoriteType type, const SongList &songs, SearchCallback callback);
void Remove(NetworkAccessManager *network, const std::string &api_url, uint64_t user_id, const std::string &country_code,
            const std::map<std::string, std::string> &headers, FavoriteType type, const SongList &songs, SearchCallback callback);

}  // namespace TidalFavoriteRequest

#endif
