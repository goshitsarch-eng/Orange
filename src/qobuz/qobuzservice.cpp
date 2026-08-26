#include "qobuz/qobuzservice.h"

#include "constants/qobuzsettings.h"
#include "core/settings.h"
#include "streaming/streamingauth.h"
#include "qobuz/qobuzfavoriterequest.h"
#include "qobuz/qobuzrequest.h"
#include "qobuz/qobuzstreamurlrequest.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <ctime>

const char QobuzService::kApiUrl[] = "https://www.qobuz.com/api.json/0.2";

QobuzService::QobuzService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

void QobuzService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(QobuzSettings::kSettingsGroup);
  app_id_ = settings.Value(QobuzSettings::kAppId);
  if (app_id_.empty()) {
    app_id_ = settings.Value("appid");
  }
  app_secret_ = settings.Value(QobuzSettings::kAppSecret);
  user_auth_token_ = settings.Value("token");
  if (user_auth_token_.empty()) {
    user_auth_token_ = settings.Value(QobuzSettings::kUserAuthToken);
  }
  format_ = settings.IntValue(QobuzSettings::kFormat, QobuzSettings::kDefaultFormat);
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
  NotifyAuthenticationChanged();
}

void QobuzService::Logout() {
  StreamingAuth::ClearKeys(QobuzSettings::kSettingsGroup, {"token", QobuzSettings::kUserAuthToken});
  user_auth_token_.clear();
  StreamingService::Logout();
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
  auto guarded = GuardSearch(std::move(callback));
  const int gen = search_generation();
  const auto request_type = QobuzRequest::FromSearchType(type);
  QobuzRequest::GetAll(
      network_,
      [this, request_type, query](int offset, int limit) {
        return QobuzRequest::Url(kApiUrl, request_type, query, app_id_, user_auth_token_, offset, limit);
      },
      AuthHeaders(), request_type, guarded, [this](int received, int total) { ReportSearchProgress(received, total); },
      [this, gen]() { return SearchRequestCurrent(gen); });
}

void QobuzService::GetArtists(SearchCallback callback) {
  auto guarded = GuardArtists(std::move(callback));
  const int gen = artists_generation();
  QobuzRequest::GetAll(
      network_,
      [this](int offset, int limit) {
        return QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteArtists, {}, app_id_, user_auth_token_, offset, limit);
      },
      AuthHeaders(), QobuzRequest::Type::FavouriteArtists, guarded,
      [this](int received, int total) { ReportArtistsProgress(received, total); }, [this, gen]() { return ArtistsRequestCurrent(gen); });
}

void QobuzService::GetAlbums(SearchCallback callback) {
  auto guarded = GuardAlbums(std::move(callback));
  const int gen = albums_generation();
  QobuzRequest::GetAll(
      network_,
      [this](int offset, int limit) {
        return QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteAlbums, {}, app_id_, user_auth_token_, offset, limit);
      },
      AuthHeaders(), QobuzRequest::Type::FavouriteAlbums, guarded, [this](int received, int total) { ReportAlbumsProgress(received, total); },
      [this, gen]() { return AlbumsRequestCurrent(gen); });
}

void QobuzService::GetSongs(SearchCallback callback) {
  auto guarded = GuardSongs(std::move(callback));
  const int gen = songs_generation();
  QobuzRequest::GetAll(
      network_,
      [this](int offset, int limit) {
        return QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteSongs, {}, app_id_, user_auth_token_, offset, limit);
      },
      AuthHeaders(), QobuzRequest::Type::FavouriteSongs, guarded, [this](int received, int total) { ReportSongsProgress(received, total); },
      [this, gen]() { return SongsRequestCurrent(gen); });
}

void QobuzService::GetArtistAlbums(const Song &artist, SearchCallback callback) {
  if (artist.artist_id().empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  QobuzRequest::Get(network_, QobuzRequest::ArtistAlbumsUrl(kApiUrl, artist.artist_id(), app_id_, user_auth_token_), AuthHeaders(),
                    QobuzRequest::Type::SearchAlbums, std::move(callback));
}

void QobuzService::GetAlbumSongs(const Song &album, SearchCallback callback) {
  if (album.album_id().empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  QobuzRequest::Get(network_, QobuzRequest::AlbumSongsUrl(kApiUrl, album.album_id(), app_id_, user_auth_token_), AuthHeaders(),
                    QobuzRequest::Type::SearchSongs, std::move(callback));
}

UrlHandler::LoadResult QobuzService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  result.media_url = url;
  const std::string id = QobuzStreamUrlRequest::TrackId(url);
  if (!network_ || id.empty() || !logged_in_) {
    result.error = "Qobuz is not signed in";
    if (callback) {
      callback(result);
    }
    return result;
  }
  result.type = LoadResult::Type::WillLoadAsynchronously;
  const uint64_t timestamp = static_cast<uint64_t>(std::time(nullptr));
  QobuzStreamUrlRequest::Get(network_,
                             QobuzStreamUrlRequest::Url(kApiUrl, id, format_, timestamp, app_id_, app_secret_, user_auth_token_),
                             AuthHeaders(), url, id, std::move(callback));
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
