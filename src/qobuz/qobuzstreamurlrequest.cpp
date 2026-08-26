#include "qobuz/qobuzstreamurlrequest.h"

#include "streaming/streamingmediaid.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <glib.h>

#include <map>
#include <vector>

namespace {

constexpr int64_t kNsecPerSec = 1000000000LL;

std::string AppendQuery(std::string url, const std::map<std::string, std::string> &params) {
  bool first = url.find('?') == std::string::npos;
  for (const auto &param : params) {
    url += (first ? "?" : "&") + StrUtils::UriEscape(param.first) + "=" + StrUtils::UriEscape(param.second);
    first = false;
  }
  return url;
}

}  // namespace

namespace QobuzStreamUrlRequest {

std::string TrackId(const std::string &url) { return StreamingMediaId(url); }

std::string Md5Hex(const std::string &value) {
  gchar *digest = g_compute_checksum_for_string(G_CHECKSUM_MD5, value.c_str(), static_cast<gssize>(value.size()));
  std::string result = digest ? digest : "";
  g_free(digest);
  return result;
}

std::string SignaturePayload(const std::string &track_id, int format_id, uint64_t timestamp, const std::string &app_secret) {
  return "trackgetFileUrlformat_id" + std::to_string(format_id) + "intentstreamtrack_id" + track_id + std::to_string(timestamp) +
         app_secret;
}

std::string Sign(const std::string &track_id, int format_id, uint64_t timestamp, const std::string &app_secret) {
  return Md5Hex(SignaturePayload(track_id, format_id, timestamp, app_secret));
}

std::string Url(const std::string &api_url, const std::string &track_id, int format_id, uint64_t timestamp, const std::string &app_id,
                const std::string &app_secret, const std::string &user_auth_token) {
  std::map<std::string, std::string> params;
  params["app_id"] = app_id;
  params["format_id"] = std::to_string(format_id);
  params["intent"] = "stream";
  params["track_id"] = track_id;
  if (!app_secret.empty()) {
    params["request_ts"] = std::to_string(timestamp);
    params["request_sig"] = Sign(track_id, format_id, timestamp, app_secret);
  }
  if (!user_auth_token.empty()) {
    params["user_auth_token"] = user_auth_token;
  }
  return AppendQuery(api_url + "/track/getFileUrl", params);
}

Song::FileType FiletypeFromMime(const std::string &mimetype) {
  const Song::FileType type = Song::FiletypeByMimeType(mimetype);
  return type == Song::FileType::Unknown ? Song::FileType::Stream : type;
}

UrlHandler::LoadResult Parse(const std::string &json, const std::string &media_url, const std::string &expected_track_id) {
  UrlHandler::LoadResult result;
  result.media_url = media_url;
  if (json.empty()) {
    result.error = "Empty json object.";
    return result;
  }
  const std::string track_id = JsonUtils::GetString(json, {"track_id"});
  if (track_id.empty()) {
    result.error = "Invalid Json reply, stream url is missing track_id.";
    return result;
  }
  if (!expected_track_id.empty() && track_id != expected_track_id) {
    result.error = "Incorrect track ID returned.";
    return result;
  }
  const std::string stream_url = JsonUtils::GetString(json, {"url"});
  const std::string mime = JsonUtils::GetString(json, {"mime_type"});
  if (stream_url.empty() || mime.empty()) {
    result.error = "Invalid Json reply, stream url is missing url or mime_type.";
    return result;
  }
  result.stream_url = stream_url;
  result.filetype = FiletypeFromMime(mime);
  const int duration_sec = JsonUtils::GetInt(json, {"duration"}, -1);
  if (duration_sec > 0) {
    result.duration = static_cast<int64_t>(duration_sec) * kNsecPerSec;
  }
  const double sampling_rate = JsonUtils::GetDouble(json, {"sampling_rate"}, -1.0);
  if (sampling_rate > 0.0) {
    result.samplerate = static_cast<int>(sampling_rate) * 1000;
  }
  const int bit_depth = JsonUtils::GetInt(json, {"bit_depth"}, -1);
  if (bit_depth > 0) {
    result.bit_depth = bit_depth;
  }
  result.type = UrlHandler::LoadResult::Type::TrackAvailable;
  return result;
}

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers,
         const std::string &media_url, const std::string &expected_track_id, UrlHandler::AsyncCallback callback) {
  if (!network || url.empty()) {
    UrlHandler::LoadResult result;
    result.media_url = media_url;
    result.error = "Qobuz stream request is incomplete";
    if (callback) {
      callback(result);
    }
    return;
  }
  network->Get(
      url,
      [media_url, expected_track_id, callback](const NetworkAccessManager::Response &response) {
        UrlHandler::LoadResult result;
        if (response.ok()) {
          result = Parse(response.body, media_url, expected_track_id);
        } else {
          result.media_url = media_url;
          result.error = response.error.empty() ? "Qobuz stream URL missing" : response.error;
        }
        if (callback) {
          callback(result);
        }
      },
      headers);
}

}  // namespace QobuzStreamUrlRequest
