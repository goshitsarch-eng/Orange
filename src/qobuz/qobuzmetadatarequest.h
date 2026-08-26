#ifndef STRAWBERRY_QOBUZMETADATAREQUEST_H
#define STRAWBERRY_QOBUZMETADATAREQUEST_H

#include "core/network.h"
#include "core/song.h"

#include <functional>
#include <map>
#include <string>

namespace QobuzMetadataRequest {

using Callback = std::function<void(const Song &, const std::string &error)>;

std::string Url(const std::string &api_url, const std::string &track_id, const std::string &app_id, const std::string &user_auth_token);
Song ParseTrack(const std::string &json);

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers,
         Callback callback);

}  // namespace QobuzMetadataRequest

#endif
