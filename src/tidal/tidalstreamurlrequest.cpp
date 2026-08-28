#include "tidal/tidalstreamurlrequest.h"

#include "streaming/streamingmediaid.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <glib.h>
#include <json-glib/json-glib.h>

namespace TidalStreamUrlRequest {

std::string TrackId(const std::string &url) { return StreamingMediaId(url); }

Method MethodFromSettings(int value) {
  switch (value) {
    case static_cast<int>(Method::UrlPostPaywall):
      return Method::UrlPostPaywall;
    case static_cast<int>(Method::PlaybackInfoPostPaywall):
      return Method::PlaybackInfoPostPaywall;
    case static_cast<int>(Method::StreamUrl):
    default:
      return Method::StreamUrl;
  }
}

std::string Resource(Method method, const std::string &track_id) {
  const std::string id = StrUtils::UriEscape(track_id);
  switch (method) {
    case Method::UrlPostPaywall:
      return "tracks/" + id + "/urlpostpaywall";
    case Method::PlaybackInfoPostPaywall:
      return "tracks/" + id + "/playbackinfopostpaywall";
    case Method::StreamUrl:
      break;
  }
  return "tracks/" + id + "/streamUrl";
}

std::string Url(const std::string &api_url, Method method, const std::string &track_id, const std::string &country_code,
                const std::string &quality) {
  std::string url = api_url + "/" + Resource(method, track_id);
  if (method == Method::StreamUrl) {
    url += "?soundQuality=" + StrUtils::UriEscape(quality);
  } else {
    url += "?audioquality=" + StrUtils::UriEscape(quality);
    url += "&playbackmode=STREAM&assetpresentation=FULL";
    if (method == Method::UrlPostPaywall) {
      url += "&urlusagemode=STREAM";
    }
  }
  url += "&countryCode=" + StrUtils::UriEscape(country_code);
  return url;
}

std::string EncodeBase64(const std::string &value) {
  gchar *encoded = g_base64_encode(reinterpret_cast<const guchar *>(value.data()), static_cast<gsize>(value.size()));
  std::string result = encoded ? encoded : "";
  g_free(encoded);
  return result;
}

std::string DecodeBase64(const std::string &value) {
  gsize length = 0;
  guchar *decoded = g_base64_decode(value.c_str(), &length);
  std::string result;
  if (decoded) {
    result.assign(reinterpret_cast<const char *>(decoded), length);
  }
  g_free(decoded);
  return result;
}

std::vector<std::string> ParseUrls(const std::string &json) {
  std::vector<std::string> urls;
  JsonParser *parser = json_parser_new();
  if (!json_parser_load_from_data(parser, json.data(), static_cast<gssize>(json.size()), nullptr)) {
    g_object_unref(parser);
    return urls;
  }
  JsonNode *root = json_parser_get_root(parser);
  if (root && JSON_NODE_HOLDS_OBJECT(root)) {
    JsonObject *object = json_node_get_object(root);
    if (json_object_has_member(object, "urls") && JSON_NODE_HOLDS_ARRAY(json_object_get_member(object, "urls"))) {
      JsonArray *array = json_object_get_array_member(object, "urls");
      const guint n = json_array_get_length(array);
      urls.reserve(n);
      for (guint i = 0; i < n; ++i) {
        JsonNode *item = json_array_get_element(array, i);
        if (item && JSON_NODE_HOLDS_VALUE(item) && json_node_get_value_type(item) == G_TYPE_STRING) {
          const char *value = json_node_get_string(item);
          if (value && *value) {
            urls.emplace_back(value);
          }
        }
      }
    }
  }
  g_object_unref(parser);
  return urls;
}

Song::FileType FiletypeFromCodecOrMime(const std::string &codec, const std::string &mimetype, const std::string &url) {
  if (!codec.empty()) {
    const Song::FileType by_codec = Song::FiletypeByExtension(codec);
    if (by_codec != Song::FileType::Unknown) {
      return by_codec;
    }
  }
  if (!mimetype.empty()) {
    const Song::FileType by_mime = Song::FiletypeByMimeType(mimetype);
    if (by_mime != Song::FileType::Unknown) {
      return by_mime;
    }
  }
  if (!url.empty()) {
    const Song::FileType by_url = Song::FiletypeByFilename(url);
    if (by_url != Song::FileType::Unknown) {
      return by_url;
    }
  }
  return Song::FileType::Stream;
}

namespace {

bool LooksLikeXml(const std::string &value) {
  size_t i = 0;
  while (i < value.size() && (value[i] == ' ' || value[i] == '\n' || value[i] == '\r' || value[i] == '\t')) {
    ++i;
  }
  return i < value.size() && value[i] == '<';
}

}  // namespace

UrlHandler::LoadResult Parse(const std::string &json, const std::string &media_url, const std::string &expected_track_id) {
  UrlHandler::LoadResult result;
  result.media_url = media_url;
  result.filetype = Song::FileType::Stream;
  if (json.empty()) {
    result.error = "Missing stream urls.";
    return result;
  }

  const std::string track_id = JsonUtils::GetString(json, {"trackId"});
  if (track_id.empty()) {
    result.error = "Invalid Json reply, stream missing trackId.";
    return result;
  }
  if (!expected_track_id.empty() && track_id != expected_track_id) {
    // Tidal sometimes returns a substituted track. Still accept the stream if a URL is present.
  }

  std::string codec = JsonUtils::GetString(json, {"codec"});
  if (codec.empty()) {
    codec = JsonUtils::GetString(json, {"codecs"});
  }
  const int samplerate = JsonUtils::GetInt(json, {"sampleRate"}, -1);
  const int bit_depth = JsonUtils::GetInt(json, {"bitDepth"}, -1);
  if (samplerate > 0) {
    result.samplerate = samplerate;
  }
  if (bit_depth > 0) {
    result.bit_depth = bit_depth;
  }

  std::vector<std::string> urls;
  const std::string manifest_b64 = JsonUtils::GetString(json, {"manifest"});
  if (!manifest_b64.empty()) {
    const std::string manifest = DecodeBase64(manifest_b64);
    if (LooksLikeXml(manifest)) {
      urls.push_back("data:application/dash+xml;base64," + manifest_b64);
    } else {
      const std::string encryption = JsonUtils::GetString(manifest, {"encryptionType"});
      if (!encryption.empty() && StrUtils::ToUpper(encryption) != "NONE") {
        result.error = "Received URL with " + encryption + " encrypted stream from Tidal. Orange does not currently support encrypted streams.";
        return result;
      }
      urls = ParseUrls(manifest);
      const std::string mime = JsonUtils::GetString(manifest, {"mimeType"});
      const std::string manifest_codec = JsonUtils::GetString(manifest, {"codecs"});
      result.filetype = FiletypeFromCodecOrMime(manifest_codec.empty() ? codec : manifest_codec, mime, urls.empty() ? std::string() : urls.front());
    }
  }

  const auto top_urls = ParseUrls(json);
  urls.insert(urls.end(), top_urls.begin(), top_urls.end());
  if (urls.empty()) {
    const std::string single = JsonUtils::GetString(json, {"url"});
    if (!single.empty()) {
      urls.push_back(single);
    }
  }

  const std::string encryption_key = JsonUtils::GetString(json, {"encryptionKey"});
  if (!encryption_key.empty()) {
    result.error = "Received URL with encrypted stream from Tidal. Orange does not currently support encrypted streams.";
    return result;
  }
  const std::string security_type = JsonUtils::GetString(json, {"securityType"});
  const std::string security_token = JsonUtils::GetString(json, {"securityToken"});
  if (!security_type.empty() && !security_token.empty()) {
    result.error = "Received URL with encrypted stream from Tidal. Orange does not currently support encrypted streams.";
    return result;
  }

  if (urls.empty()) {
    result.error = "Missing stream urls.";
    return result;
  }

  result.stream_url = urls.front();
  if (result.filetype == Song::FileType::Stream || result.filetype == Song::FileType::Unknown) {
    result.filetype = FiletypeFromCodecOrMime(codec, {}, result.stream_url);
  }
  result.type = UrlHandler::LoadResult::Type::TrackAvailable;
  return result;
}

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers,
         const std::string &media_url, const std::string &expected_track_id, UrlHandler::AsyncCallback callback) {
  if (!network || url.empty()) {
    UrlHandler::LoadResult result;
    result.media_url = media_url;
    result.error = "Tidal stream request is incomplete";
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
          result.error = response.error.empty() ? "Tidal stream URL missing" : response.error;
        }
        if (callback) {
          callback(result);
        }
      },
      headers);
}

}  // namespace TidalStreamUrlRequest
