#ifndef STRAWBERRY_TIDALSTREAMURLREQUEST_H
#define STRAWBERRY_TIDALSTREAMURLREQUEST_H

#include "constants/tidalsettings.h"
#include "core/network.h"
#include "core/urlhandlers.h"

#include <map>
#include <string>
#include <vector>

namespace TidalStreamUrlRequest {

using Method = TidalSettings::StreamUrlMethod;

std::string TrackId(const std::string &url);
Method MethodFromSettings(int value);
std::string Resource(Method method, const std::string &track_id);
std::string Url(const std::string &api_url, Method method, const std::string &track_id, const std::string &country_code,
                const std::string &quality);

std::string EncodeBase64(const std::string &value);
std::string DecodeBase64(const std::string &value);
std::vector<std::string> ParseUrls(const std::string &json);
Song::FileType FiletypeFromCodecOrMime(const std::string &codec, const std::string &mimetype, const std::string &url);
UrlHandler::LoadResult Parse(const std::string &json, const std::string &media_url, const std::string &expected_track_id = {});

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers,
         const std::string &media_url, const std::string &expected_track_id, UrlHandler::AsyncCallback callback);

}  // namespace TidalStreamUrlRequest

#endif
