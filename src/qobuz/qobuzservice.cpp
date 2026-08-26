#include "qobuz/qobuzservice.h"

#include "core/settings.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

const char QobuzService::kApiUrl[] = "https://www.qobuz.com/api.json/0.2";

QobuzService::QobuzService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

void QobuzService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup("Qobuz");
  app_id_ = settings.Value("appid");
  user_auth_token_ = settings.Value("token");
  if (user_auth_token_.empty()) {
    user_auth_token_ = settings.Value("user_auth_token");
  }
  logged_in_ = !app_id_.empty() && !user_auth_token_.empty();
}

void QobuzService::Login(const std::string &username, const std::string &password_or_token) {
  Settings settings;
  settings.BeginGroup("Qobuz");
  if (!username.empty()) {
    settings.SetValue("username", username);
  }
  settings.SetValue("token", password_or_token);
  settings.SetValue("user_auth_token", password_or_token);
  settings.Sync();
  ReloadSettings();
}

std::map<std::string, std::string> QobuzService::AuthHeaders() const {
  std::map<std::string, std::string> headers;
  if (!app_id_.empty()) {
    headers["X-App-Id"] = app_id_;
  }
  if (!user_auth_token_.empty()) {
    headers["X-User-Auth-Token"] = user_auth_token_;
  }
  return headers;
}

void QobuzService::Search(const std::string &query, SearchCallback callback) {
  if (!network_ || query.empty()) {
    callback({});
    return;
  }
  std::string url = std::string(kApiUrl) + "/track/search?query=" + StrUtils::UriEscape(query) + "&limit=50";
  if (!app_id_.empty()) {
    url += "&app_id=" + StrUtils::UriEscape(app_id_);
  }
  if (!user_auth_token_.empty()) {
    url += "&user_auth_token=" + StrUtils::UriEscape(user_auth_token_);
  }
  network_->Get(url, [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({});
      return;
    }
    callback(JsonUtils::ParseQobuzTracks(response.body));
  }, AuthHeaders());
}

UrlHandler::LoadResult QobuzService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  result.media_url = url;
  std::string id = url;
  const auto scheme = id.find("://");
  if (scheme != std::string::npos) {
    id = id.substr(scheme + 3);
  }
  if (!network_ || id.empty() || !logged_in_) {
    result.error = "Qobuz is not signed in";
    if (callback) {
      callback(result);
    }
    return result;
  }
  result.type = LoadResult::Type::Async;
  const std::string request = std::string(kApiUrl) + "/track/getFileUrl?track_id=" + StrUtils::UriEscape(id) +
                              "&format_id=5&app_id=" + StrUtils::UriEscape(app_id_) +
                              "&user_auth_token=" + StrUtils::UriEscape(user_auth_token_);
  network_->Get(request, [callback, url](const NetworkAccessManager::Response &response) {
    LoadResult async;
    async.media_url = url;
    if (response.ok()) {
      async.stream_url = JsonUtils::GetString(response.body, {"url"});
      if (async.stream_url.empty()) {
        async.stream_url = JsonUtils::FindStringByKeys(response.body, {"url", "sample_url"});
      }
    }
    async.type = async.stream_url.empty() ? LoadResult::Type::Error : LoadResult::Type::TrackAvailable;
    if (async.stream_url.empty()) {
      async.error = response.error.empty() ? "Qobuz stream URL missing" : response.error;
    }
    if (callback) {
      callback(async);
    }
  }, AuthHeaders());
  return result;
}
