#ifndef STRAWBERRY_QOBUZFAVORITEREQUEST_H
#define STRAWBERRY_QOBUZFAVORITEREQUEST_H

#include "core/network.h"
#include "core/song.h"
#include "streaming/streamingpage.h"
#include "streaming/streamingservices.h"

#include <map>
#include <string>
#include <vector>

namespace QobuzFavoriteRequest {

using FavoriteType = StreamingService::FavoriteType;
using SearchCallback = StreamingService::SearchCallback;

std::string FavoriteText(FavoriteType type);
std::string FavoriteMethod(FavoriteType type);
std::vector<std::string> IdsFromSongs(FavoriteType type, const SongList &songs);

std::string ListUrl(const std::string &api_url, FavoriteType type, const std::string &app_id, const std::string &user_auth_token,
                    int offset = 0, int limit = 50);
std::string CreateUrl(const std::string &api_url, FavoriteType type, const std::vector<std::string> &ids, const std::string &app_id,
                      const std::string &user_auth_token);
std::string DeleteUrl(const std::string &api_url, FavoriteType type, const std::vector<std::string> &ids, const std::string &app_id,
                      const std::string &user_auth_token);

SongList Parse(FavoriteType type, const std::string &json);

void Get(NetworkAccessManager *network, const std::string &api_url, const std::string &app_id, const std::string &user_auth_token,
         const std::map<std::string, std::string> &headers, FavoriteType type, SearchCallback callback,
         StreamingPage::ProgressCallback progress = {}, StreamingPage::StillCurrent still_current = {},
         StreamingPage::ErrorCallback error = {});
void Add(NetworkAccessManager *network, const std::string &api_url, const std::string &app_id, const std::string &user_auth_token,
         const std::map<std::string, std::string> &headers, FavoriteType type, const SongList &songs, SearchCallback callback);
void Remove(NetworkAccessManager *network, const std::string &api_url, const std::string &app_id, const std::string &user_auth_token,
            const std::map<std::string, std::string> &headers, FavoriteType type, const SongList &songs, SearchCallback callback);

}  // namespace QobuzFavoriteRequest

#endif
