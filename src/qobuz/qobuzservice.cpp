#include "qobuz/qobuzservice.h"

#include "core/settings.h"
#include "qobuz/qobuzfavoriterequest.h"
#include "qobuz/qobuzrequest.h"
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

void QobuzService::Search(const std::string &query, SearchCallback callback) { Search(query, SearchType::Songs, std::move(callback)); }

void QobuzService::Search(const std::string &query, SearchType type, SearchCallback callback) {
  const auto request_type = QobuzRequest::FromSearchType(type);
  QobuzRequest::Get(network_, QobuzRequest::Url(kApiUrl, request_type, query, app_id_, user_auth_token_), AuthHeaders(), request_type,
                    std::move(callback));
}

void QobuzService::GetArtists(SearchCallback callback) {
  QobuzRequest::Get(network_, QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteArtists, {}, app_id_, user_auth_token_), AuthHeaders(),
                    QobuzRequest::Type::FavouriteArtists, std::move(callback));
}

void QobuzService::GetAlbums(SearchCallback callback) {
  QobuzRequest::Get(network_, QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteAlbums, {}, app_id_, user_auth_token_), AuthHeaders(),
                    QobuzRequest::Type::FavouriteAlbums, std::move(callback));
}

void QobuzService::GetSongs(SearchCallback callback) {
  QobuzRequest::Get(network_, QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteSongs, {}, app_id_, user_auth_token_), AuthHeaders(),
                    QobuzRequest::Type::FavouriteSongs, std::move(callback));
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

void QobuzService::GetFavorites(FavoriteType type, SearchCallback callback) {
  QobuzFavoriteRequest::Get(network_, kApiUrl, app_id_, user_auth_token_, AuthHeaders(), type, std::move(callback));
}

void QobuzService::AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  QobuzFavoriteRequest::Add(network_, kApiUrl, app_id_, user_auth_token_, AuthHeaders(), type, songs, std::move(callback));
}

void QobuzService::RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  QobuzFavoriteRequest::Remove(network_, kApiUrl, app_id_, user_auth_token_, AuthHeaders(), type, songs, std::move(callback));
}
