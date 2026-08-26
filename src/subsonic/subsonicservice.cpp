#include "subsonic/subsonicservice.h"

#include "core/settings.h"
#include "subsonic/subsonicfavoriterequest.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <glib.h>

const char *SubsonicService::kClientName = "Strawberry";
const char *SubsonicService::kApiVersion = "1.11.0";

namespace {

std::string JoinPath(const std::string &server, const std::string &resource) {
  std::string url = server;
  if (!url.empty() && url.back() == '/') {
    url.pop_back();
  }
  return url + "/rest/" + resource + ".view";
}

}  // namespace

SubsonicService::SubsonicService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

std::string SubsonicService::Md5Hex(const std::string &value) {
  gchar *digest = g_compute_checksum_for_string(G_CHECKSUM_MD5, value.c_str(), static_cast<gssize>(value.size()));
  std::string result = digest ? digest : "";
  g_free(digest);
  return result;
}

std::string SubsonicService::HexEncode(const std::string &value) {
  std::string hex;
  hex.reserve(value.size() * 2);
  for (unsigned char ch : value) {
    char buf[3];
    g_snprintf(buf, sizeof(buf), "%02x", ch);
    hex += buf;
  }
  return hex;
}

std::string SubsonicService::RandomSalt(int length) {
  static const char alphabet[] = "abcdefghijklmnopqrstuvwxyz0123456789";
  std::string salt;
  salt.reserve(static_cast<size_t>(length));
  for (int i = 0; i < length; ++i) {
    salt.push_back(alphabet[g_random_int_range(0, static_cast<gint32>(sizeof(alphabet) - 1))]);
  }
  return salt;
}

std::string SubsonicService::CreateUrl(const std::string &server_url, const std::string &username, const std::string &password,
                                       const std::string &resource, const std::map<std::string, std::string> &params, bool hex_auth) {
  std::string url = JoinPath(server_url, resource);
  url += "?c=" + StrUtils::UriEscape(kClientName) + "&v=" + kApiVersion + "&f=json&u=" + StrUtils::UriEscape(username);
  if (hex_auth) {
    url += "&p=enc:" + HexEncode(password);
  } else {
    const std::string salt = RandomSalt();
    url += "&s=" + salt + "&t=" + Md5Hex(password + salt);
  }
  for (const auto &param : params) {
    url += "&" + StrUtils::UriEscape(param.first) + "=" + StrUtils::UriEscape(param.second);
  }
  return url;
}

void SubsonicService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup("Subsonic");
  server_url_ = settings.Value("url");
  username_ = settings.Value("username");
  password_ = settings.Value("password");
  if (password_.empty()) {
    password_ = settings.Value("token");
  }
  hex_auth_ = settings.Value("auth") == "hex";
  logged_in_ = !server_url_.empty() && !username_.empty() && !password_.empty();
}

void SubsonicService::Login(const std::string &username, const std::string &password_or_token) {
  Settings settings;
  settings.BeginGroup("Subsonic");
  settings.SetValue("username", username);
  settings.SetValue("password", password_or_token);
  settings.SetValue("token", password_or_token);
  settings.Sync();
  ReloadSettings();
}

void SubsonicService::Search(const std::string &query, SearchCallback callback) {
  if (!network_ || !logged_in_ || query.empty()) {
    callback({});
    return;
  }
  const std::string url = CreateUrl(server_url_, username_, password_, "search3",
                                    {{"query", query}, {"songCount", "50"}, {"albumCount", "0"}, {"artistCount", "0"}}, hex_auth_);
  network_->Get(url, [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({});
      return;
    }
    callback(JsonUtils::ParseSubsonicSongs(response.body));
  });
}

UrlHandler::LoadResult SubsonicService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  std::string id = url;
  const auto scheme = id.find("://");
  if (scheme != std::string::npos) {
    id = id.substr(scheme + 3);
  }
  if (!id.empty() && id.front() == '/') {
    id.erase(id.begin());
  }
  if (!logged_in_ || id.empty()) {
    result.error = "Subsonic is not signed in";
    if (callback) {
      callback(result);
    }
    return result;
  }
  result.type = LoadResult::Type::TrackAvailable;
  result.media_url = url;
  result.stream_url = CreateUrl(server_url_, username_, password_, "stream", {{"id", id}}, hex_auth_);
  if (callback) {
    callback(result);
  }
  return result;
}

void SubsonicService::GetFavorites(FavoriteType, SearchCallback callback) {
  if (!logged_in_) {
    if (callback) {
      callback({});
    }
    return;
  }
  SubsonicFavoriteRequest::Get(network_, CreateUrl(server_url_, username_, password_, "getStarred2", {}, hex_auth_), std::move(callback));
}

void SubsonicService::AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  const auto ids = SubsonicFavoriteRequest::IdsFromSongs(type, songs);
  if (!logged_in_ || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  SubsonicFavoriteRequest::Mutate(network_,
                                  CreateUrl(server_url_, username_, password_, SubsonicFavoriteRequest::StarResource(false),
                                            SubsonicFavoriteRequest::StarParams(type, ids.front()), hex_auth_),
                                  songs, std::move(callback));
}

void SubsonicService::RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  const auto ids = SubsonicFavoriteRequest::IdsFromSongs(type, songs);
  if (!logged_in_ || ids.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  SubsonicFavoriteRequest::Mutate(network_,
                                  CreateUrl(server_url_, username_, password_, SubsonicFavoriteRequest::StarResource(true),
                                            SubsonicFavoriteRequest::StarParams(type, ids.front()), hex_auth_),
                                  songs, std::move(callback));
}
