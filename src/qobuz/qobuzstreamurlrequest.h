#ifndef STRAWBERRY_QOBUZSTREAMURLREQUEST_H
#define STRAWBERRY_QOBUZSTREAMURLREQUEST_H

#include "core/network.h"
#include "core/urlhandlers.h"

#include <cstdint>
#include <map>
#include <string>

namespace QobuzStreamUrlRequest {

std::string TrackId(const std::string &url);
std::string Md5Hex(const std::string &value);
std::string SignaturePayload(const std::string &track_id, int format_id, uint64_t timestamp, const std::string &app_secret);
std::string Sign(const std::string &track_id, int format_id, uint64_t timestamp, const std::string &app_secret);
std::string Url(const std::string &api_url, const std::string &track_id, int format_id, uint64_t timestamp, const std::string &app_id,
                const std::string &app_secret, const std::string &user_auth_token);
Song::FileType FiletypeFromMime(const std::string &mimetype);
UrlHandler::LoadResult Parse(const std::string &json, const std::string &media_url, const std::string &expected_track_id);

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers,
         const std::string &media_url, const std::string &expected_track_id, UrlHandler::AsyncCallback callback);

}  // namespace QobuzStreamUrlRequest

#endif
