#include "tidal/tidalservice.h"

#include "core/settings.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

const char TidalService::kApiUrl[] = "https://api.tidalhifi.com/v1";
const char TidalService::kResourcesUrl[] = "https://resources.tidal.com";

TidalService::TidalService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

void TidalService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup("Tidal");
  token_ = settings.Value("token");
  if (token_.empty()) {
    token_ = settings.Value("access_token");
  }
  country_code_ = settings.Value("countrycode", "US");
  logged_in_ = !token_.empty();
}

void TidalService::Login(const std::string &username, const std::string &password_or_token) {
  Settings settings;
  settings.BeginGroup("Tidal");
  if (!username.empty()) {
    settings.SetValue("username", username);
  }
  settings.SetValue("token", password_or_token);
  settings.SetValue("access_token", password_or_token);
  settings.Sync();
  ReloadSettings();
}

std::map<std::string, std::string> TidalService::AuthHeaders() const {
  if (token_.empty()) {
    return {};
  }
  return {{"Authorization", "Bearer " + token_}};
}

std::string TidalService::TrackId(const std::string &url) const {
  std::string id = url;
  const auto scheme = id.find("://");
  if (scheme != std::string::npos) {
    id = id.substr(scheme + 3);
  }
  if (!id.empty() && id.front() == '/') {
    id.erase(id.begin());
  }
  return id;
}

void TidalService::Search(const std::string &query, SearchCallback callback) {
  if (!network_ || query.empty()) {
    callback({});
    return;
  }
  const std::string url = std::string(kApiUrl) + "/search/tracks?query=" + StrUtils::UriEscape(query) +
                          "&limit=50&countryCode=" + StrUtils::UriEscape(country_code_);
  network_->Get(url, [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({});
      return;
    }
    callback(JsonUtils::ParseTidalTracks(response.body));
  }, AuthHeaders());
}

UrlHandler::LoadResult TidalService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  result.media_url = url;
  const std::string id = TrackId(url);
  if (!network_ || id.empty() || token_.empty()) {
    result.error = "Tidal is not signed in";
    if (callback) {
      callback(result);
    }
    return result;
  }
  result.type = LoadResult::Type::Async;
  const std::string stream = std::string(kApiUrl) + "/tracks/" + StrUtils::UriEscape(id) +
                             "/streamUrl?countryCode=" + StrUtils::UriEscape(country_code_);
  network_->Get(stream, [callback, url](const NetworkAccessManager::Response &response) {
    LoadResult async;
    async.media_url = url;
    if (response.ok()) {
      async.stream_url = JsonUtils::GetString(response.body, {"url"});
      if (async.stream_url.empty()) {
        async.stream_url = JsonUtils::FindStringByKeys(response.body, {"url", "urls", "manifest"});
      }
    }
    async.type = async.stream_url.empty() ? LoadResult::Type::Error : LoadResult::Type::TrackAvailable;
    if (async.stream_url.empty()) {
      async.error = response.error.empty() ? "Tidal stream URL missing" : response.error;
    }
    if (callback) {
      callback(async);
    }
  }, AuthHeaders());
  return result;
}
