#include "qobuz/qobuzservice.h"

#include "constants/qobuzsettings.h"
#include "core/localredirectserver.h"
#include "core/logging.h"
#include "core/network.h"
#include "core/settings.h"
#include "qobuz/qobuzoauth.h"
#include "streaming/streamingalbum.h"
#include "streaming/streamingauth.h"
#include "streaming/streamingsearchopts.h"
#include "qobuz/qobuzfavoriterequest.h"
#include "qobuz/qobuzmetadatarequest.h"
#include "qobuz/qobuzrequest.h"
#include "qobuz/qobuzstreamurlrequest.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <gio/gio.h>

#include <ctime>

const char QobuzService::kApiUrl[] = "https://www.qobuz.com/api.json/0.2";

QobuzService::QobuzService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

QobuzService::~QobuzService() { CloseRedirectServer(); }

void QobuzService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(QobuzSettings::kSettingsGroup);
  app_id_ = settings.Value(QobuzSettings::kAppId);
  if (app_id_.empty()) {
    app_id_ = settings.Value("appid");
  }
  app_secret_ = settings.Value(QobuzSettings::kAppSecret);
  private_key_ = settings.Value(QobuzSettings::kPrivateKey);
  user_id_ = settings.Int64Value(QobuzSettings::kUserId, -1);
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

void QobuzService::Authenticate(const std::string &app_id, const std::string &app_secret, const std::string &private_key) {
  app_id_ = app_id;
  app_secret_ = app_secret;
  private_key_ = private_key;
  Authenticate();
}

void QobuzService::Authenticate() {
  if (const char *missing = QobuzOAuth::MissingCredential(app_id_, app_secret_, private_key_)) {
    NotifyAuthenticationFailed(missing);
    return;
  }
  if (!network_) {
    NotifyAuthenticationFailed("No network");
    return;
  }
  CloseRedirectServer();
  redirect_server_ = std::make_unique<LocalRedirectServer>();
  if (!redirect_server_->Listen()) {
    NotifyAuthenticationFailed(QobuzOAuth::ListenFailedMessage(redirect_server_->error()));
    CloseRedirectServer();
    return;
  }
  redirect_server_->Redirected.Connect([this](const std::string &url) { OAuthRedirectReceived(url); });
  const std::string url = QobuzOAuth::AuthorizationUrl(app_id_, redirect_server_->port());
  if (!g_app_info_launch_default_for_uri(url.c_str(), nullptr, nullptr)) {
    LogError("Qobuz: Failed to open URL %s", url.c_str());
    NotifyAuthenticationFailed(QobuzOAuth::BrowserFailedMessage(url));
    CloseRedirectServer();
  }
}

void QobuzService::CloseRedirectServer() { redirect_server_.reset(); }

void QobuzService::OAuthRedirectReceived(const std::string &url) {
  CloseRedirectServer();
  const std::string code = QobuzOAuth::ExtractCode(url);
  if (code.empty()) {
    NotifyAuthenticationFailed(QobuzOAuth::kMissingCode);
    return;
  }
  ExchangeCode(code);
}

void QobuzService::ExchangeCode(const std::string &code) {
  if (!network_) {
    NotifyAuthenticationFailed("No network");
    return;
  }
  const std::string callback = QobuzOAuth::CallbackRequestUrl(code, private_key_);
  network_->Get(callback,
                [this](const NetworkAccessManager::Response &response) {
                  HandleOAuthCallback(response.body, response.error, response.status);
                },
                {{"X-App-Id", app_id_}});
}

void QobuzService::HandleOAuthCallback(const std::string &body, const std::string &error, unsigned status) {
  if (!error.empty() && status == 0) {
    NotifyAuthenticationFailed(error);
    return;
  }
  if (status != 200) {
    const std::string message = JsonUtils::GetString(body, {"message"});
    const int code = JsonUtils::GetInt(body, {"code"}, 0);
    if (!message.empty()) {
      NotifyAuthenticationFailed(QobuzOAuth::ApiErrorMessage(message, code));
      return;
    }
    NotifyAuthenticationFailed(error.empty() ? QobuzOAuth::HttpErrorMessage(status) : error);
    return;
  }
  if (body.empty() || body.find('{') == std::string::npos) {
    NotifyAuthenticationFailed(QobuzOAuth::kMissingJson);
    return;
  }
  const std::string token = QobuzOAuth::PreferToken(JsonUtils::GetString(body, {"token"}), JsonUtils::GetString(body, {"user_auth_token"}));
  if (token.empty()) {
    NotifyAuthenticationFailed(QobuzOAuth::kMissingToken);
    return;
  }
  user_auth_token_ = token;
  const int user_id = JsonUtils::GetInt(body, {"userId"}, -1);
  if (user_id > 0) {
    user_id_ = user_id;
  }
  Settings settings;
  settings.BeginGroup(QobuzSettings::kSettingsGroup);
  settings.SetValue(QobuzSettings::kUserAuthToken, user_auth_token_);
  settings.SetValue("token", user_auth_token_);
  if (user_id_ > 0) {
    settings.SetInt64Value(QobuzSettings::kUserId, user_id_);
  }
  settings.Sync();
  ReloadSettings();
  NotifyAuthenticationChanged();
}

void QobuzService::Logout() {
  StreamingAuth::ClearKeys(QobuzSettings::kSettingsGroup, {"token", QobuzSettings::kUserAuthToken, QobuzSettings::kUserId});
  user_auth_token_.clear();
  user_id_ = -1;
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
  const int limit = StreamingSearchOpts::LimitFor(name(), type);
  QobuzRequest::GetAll(
      network_,
      [this, request_type, query](int offset, int page_limit) {
        return QobuzRequest::Url(kApiUrl, request_type, query, app_id_, user_auth_token_, offset, page_limit);
      },
      AuthHeaders(), request_type,
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, AuthHeaders(), songs, guarded,
                          [this](const std::string &text) { SearchUpdateStatus.Emit(last_search_id_, text); },
                          [this](int received, int total) { ReportSearchProgress(received, total); },
                          [this, gen]() { return SearchRequestCurrent(gen); });
      },
      [this](int received, int total) { ReportSearchProgress(received, total); }, [this, gen]() { return SearchRequestCurrent(gen); }, limit,
      limit, [this](const std::string &error) { NotifySearchFailed(error); });
}

void QobuzService::GetArtists(SearchCallback callback) {
  auto guarded = GuardArtists(std::move(callback));
  const int gen = artists_generation();
  QobuzRequest::GetAll(
      network_,
      [this](int offset, int limit) {
        return QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteArtists, {}, app_id_, user_auth_token_, offset, limit);
      },
      AuthHeaders(), QobuzRequest::Type::FavouriteArtists,
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, AuthHeaders(), songs, guarded, [this](const std::string &text) { ArtistsUpdateStatus.Emit(text); },
                          [this](int received, int total) { ReportArtistsProgress(received, total); },
                          [this, gen]() { return ArtistsRequestCurrent(gen); });
      },
      [this](int received, int total) { ReportArtistsProgress(received, total); }, [this, gen]() { return ArtistsRequestCurrent(gen); },
      StreamingPage::kDefaultLimit, 0, [this](const std::string &error) { NotifyArtistsFailed(error); });
}

void QobuzService::GetAlbums(SearchCallback callback) {
  auto guarded = GuardAlbums(std::move(callback));
  const int gen = albums_generation();
  QobuzRequest::GetAll(
      network_,
      [this](int offset, int limit) {
        return QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteAlbums, {}, app_id_, user_auth_token_, offset, limit);
      },
      AuthHeaders(), QobuzRequest::Type::FavouriteAlbums,
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, AuthHeaders(), songs, guarded, [this](const std::string &text) { AlbumsUpdateStatus.Emit(text); },
                          [this](int received, int total) { ReportAlbumsProgress(received, total); },
                          [this, gen]() { return AlbumsRequestCurrent(gen); });
      },
      [this](int received, int total) { ReportAlbumsProgress(received, total); }, [this, gen]() { return AlbumsRequestCurrent(gen); },
      StreamingPage::kDefaultLimit, 0, [this](const std::string &error) { NotifyAlbumsFailed(error); });
}

void QobuzService::GetSongs(SearchCallback callback) {
  auto guarded = GuardSongs(std::move(callback));
  const int gen = songs_generation();
  QobuzRequest::GetAll(
      network_,
      [this](int offset, int limit) {
        return QobuzRequest::Url(kApiUrl, QobuzRequest::Type::FavouriteSongs, {}, app_id_, user_auth_token_, offset, limit);
      },
      AuthHeaders(), QobuzRequest::Type::FavouriteSongs,
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, AuthHeaders(), songs, guarded, [this](const std::string &text) { SongsUpdateStatus.Emit(text); },
                          [this](int received, int total) { ReportSongsProgress(received, total); },
                          [this, gen]() { return SongsRequestCurrent(gen); });
      },
      [this](int received, int total) { ReportSongsProgress(received, total); }, [this, gen]() { return SongsRequestCurrent(gen); },
      StreamingPage::kDefaultLimit, 0, [this](const std::string &error) { NotifySongsFailed(error); });
}

void QobuzService::GetArtistAlbums(const Song &artist, SearchCallback callback) {
  if (artist.artist_id().empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  const std::string id = artist.artist_id();
  QobuzRequest::GetAll(
      network_, [this, id](int offset, int limit) { return QobuzRequest::ArtistAlbumsUrl(kApiUrl, id, app_id_, user_auth_token_, offset, limit); },
      AuthHeaders(), QobuzRequest::Type::SearchAlbums,
      [this, callback](const SongList &songs) { DeliverWithCovers(network_, AuthHeaders(), songs, callback); },
      [this](int received, int total) { ReportAlbumsProgress(received, total); }, {}, StreamingPage::kDefaultLimit, 0,
      [this](const std::string &error) { NotifyAlbumsFailed(error); });
}

void QobuzService::GetAlbumSongs(const Song &album, SearchCallback callback) {
  if (album.album_id().empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  const std::string id = album.album_id();
  QobuzRequest::GetAll(
      network_, [this, id](int offset, int limit) { return QobuzRequest::AlbumSongsUrl(kApiUrl, id, app_id_, user_auth_token_, offset, limit); },
      AuthHeaders(), QobuzRequest::Type::SearchSongs,
      [this, album, callback](const SongList &songs) {
        SongList copy = songs;
        StreamingAlbum::ApplyParent(copy, album);
        DeliverWithCovers(network_, AuthHeaders(), copy, callback);
      },
      [this](int received, int total) { ReportSongsProgress(received, total); }, {}, StreamingPage::kDefaultLimit, 0,
      [this](const std::string &error) { NotifySongsFailed(error); });
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

void QobuzService::FetchTrackMetadata(const std::string &track_id, std::function<void(const Song &, const std::string &error)> callback) {
  if (!logged_in_) {
    if (callback) {
      callback(Song(), "Not authenticated");
    }
    return;
  }
  if (track_id.empty()) {
    if (callback) {
      callback(Song(), "No track ID");
    }
    return;
  }
  QobuzMetadataRequest::Get(network_, QobuzMetadataRequest::Url(kApiUrl, track_id, app_id_, user_auth_token_), AuthHeaders(), std::move(callback));
}

void QobuzService::GetFavorites(FavoriteType type, SearchCallback callback) {
  auto guarded = GuardFavorites(std::move(callback));
  const int gen = favorites_generation();
  QobuzFavoriteRequest::Get(
      network_, kApiUrl, app_id_, user_auth_token_, AuthHeaders(), type,
      [this, guarded, gen](const SongList &songs) {
        DeliverWithCovers(network_, AuthHeaders(), songs, guarded, [this](const std::string &text) { FavoritesUpdateStatus.Emit(text); },
                          [this](int received, int total) { ReportFavoritesProgress(received, total); },
                          [this, gen]() { return FavoritesRequestCurrent(gen); });
      },
      [this](int received, int total) { ReportFavoritesProgress(received, total); }, [this, gen]() { return FavoritesRequestCurrent(gen); },
      [this](const std::string &error) { NotifyFavoritesFailed(error); });
}

void QobuzService::AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  QobuzFavoriteRequest::Add(network_, kApiUrl, app_id_, user_auth_token_, AuthHeaders(), type, songs, std::move(callback));
}

void QobuzService::RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  QobuzFavoriteRequest::Remove(network_, kApiUrl, app_id_, user_auth_token_, AuthHeaders(), type, songs, std::move(callback));
}
