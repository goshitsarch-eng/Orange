#ifndef STRAWBERRY_SUBSONICFAVORITEREQUEST_H
#define STRAWBERRY_SUBSONICFAVORITEREQUEST_H

#include "core/network.h"
#include "core/song.h"
#include "streaming/streamingpage.h"
#include "streaming/streamingservices.h"

#include <map>
#include <string>
#include <vector>

namespace SubsonicFavoriteRequest {

using FavoriteType = StreamingService::FavoriteType;
using SearchCallback = StreamingService::SearchCallback;

std::string IdParam(FavoriteType type);
std::vector<std::string> IdsFromSongs(FavoriteType type, const SongList &songs);

std::string StarResource(bool remove);
std::map<std::string, std::string> StarParams(FavoriteType type, const std::string &id);
inline std::vector<std::map<std::string, std::string>> StarParamsForIds(FavoriteType type, const std::vector<std::string> &ids) {
  std::vector<std::map<std::string, std::string>> params;
  params.reserve(ids.size());
  for (const std::string &id : ids) {
    params.push_back(StarParams(type, id));
  }
  return params;
}

void Get(NetworkAccessManager *network, const std::string &list_url, SearchCallback callback,
         StreamingPage::ErrorCallback error = {});
void Mutate(NetworkAccessManager *network, const std::string &url, const SongList &songs, SearchCallback callback);
void MutateMany(NetworkAccessManager *network, const std::vector<std::string> &urls, const SongList &songs, SearchCallback callback);

}  // namespace SubsonicFavoriteRequest

#endif
