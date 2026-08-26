#include "tidal/tidalservice.h"

#include "constants/tidalsettings.h"
#include "core/settings.h"
#include "streaming/streamingauth.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingsearchopts.h"
#include "tidal/tidalfavoriterequest.h"
#include "tidal/tidalrequest.h"
#include "tidal/tidalstreamurlrequest.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <cstdlib>
#include <ctime>

const char TidalService::kApiUrl[] = "https://api.tidalhifi.com/v1";
const char TidalService::kResourcesUrl[] = "https://resources.tidal.com";

TidalService::TidalService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

void TidalService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(TidalSettings::kSettingsGroup);
  token_ = settings.Value("token");
  if (token_.empty()) {
    token_ = settings.Value(TidalSettings::kAccessToken);
  }
  refresh_token_ = settings.Value(TidalSettings::kRefreshToken);
  expires_in_ = settings.IntValue(TidalSettings::kExpiresIn);
  login_time_ = settings.Int64Value(TidalSettings::kLoginTime);
  client_id_ = settings.Value(TidalSettings::kClientId);
  client_secret_ = settings.Value("clientsecret");
  country_code_ = settings.Value("countrycode", "US");
  quality_ = settings.Value(TidalSettings::kQuality, TidalSettings::kDefaultQuality);
  stream_url_method_ = TidalStreamUrlRequest::MethodFromSettings(
      settings.IntValue(TidalSettings::kStreamUrl, static_cast<int>(TidalSettings::kDefaultStreamUrl)));
  const std::string user_id = settings.Value("user_id");
  user_id_ = user_id.empty() ? 0 : static_cast<uint64_t>(std::strtoull(user_id.c_str(), nullptr, 10));
  logged_in_ = !token_.empty();
}

void TidalService::StoreTokens(const OAuthenticator::TokenResponse &tokens) {
  Settings settings;
  settings.BeginGroup(TidalSettings::kSettingsGroup);
  if (!tokens.access_token.empty()) {
    settings.SetValue("token", tokens.access_token);
    settings.SetValue(TidalSettings::kAccessToken, tokens.access_token);
  }
  if (!tokens.refresh_token.empty()) {
    settings.SetValue(TidalSettings::kRefreshToken, tokens.refresh_token);
  }
  if (tokens.expires_in > 0) {
    settings.SetIntValue(TidalSettings::kExpiresIn, tokens.expires_in);
    settings.SetInt64Value(TidalSettings::kLoginTime, static_cast<gint64>(std::time(nullptr)));
  }
  settings.Sync();
  ReloadSettings();
  NotifyAuthenticationChanged();
}

void TidalService::Logout() {
  StreamingAuth::ClearKeys(TidalSettings::kSettingsGroup,
                           {"token", TidalSettings::kAccessToken, TidalSettings::kRefreshToken, TidalSettings::kExpiresIn,
                            TidalSettings::kLoginTime});
  token_.clear();
  refresh_token_.clear();
  expires_in_ = 0;
  login_time_ = 0;
  StreamingService::Logout();
}

void TidalService::Login(const std::string &username, const std::string &password_or_token) {
  Settings settings;
  settings.BeginGroup(TidalSettings::kSettingsGroup);
  if (!username.empty()) {
    settings.SetValue("username", username);
  }
  settings.SetValue("token", password_or_token);
  settings.SetValue(TidalSettings::kAccessToken, password_or_token);
  settings.Sync();
  ReloadSettings();
  NotifyAuthenticationChanged();
}

void TidalService::EnsureFreshToken(std::function<void()> next) {
  if (StreamingAuth::EnsureAction(login_time_, expires_in_, refresh_token_) == StreamingAuth::Action::Proceed) {
    if (next) {
      next();
    }
    return;
  }
  auto *oauth = new OAuthenticator(network_);
  oauth->RefreshAccessToken("https://auth.tidal.com/v1/oauth2/token", client_id_, client_secret_, refresh_token_,
                            [this, oauth, next](const std::string &body, const std::string &error) {
                              delete oauth;
                              if (!error.empty()) {
                                Logout();
                                NotifyAuthenticationFailed(error);
                                return;
                              }
                              StoreTokens(OAuthenticator::ParseTokenResponse(body));
                              if (next) {
                                next();
                              }
                            });
}

std::map<std::string, std::string> TidalService::AuthHeaders() const {
  if (token_.empty()) {
    return {};
  }
  return {{"Authorization", "Bearer " + token_}};
}

void TidalService::Search(const std::string &query, SearchCallback callback) { Search(query, SearchType::Songs, std::move(callback)); }

void TidalService::Search(const std::string &query, SearchType type, SearchCallback callback) {
  auto guarded = GuardSearch(std::move(callback));
  const int gen = search_generation();
  EnsureFreshToken([this, query, type, guarded, gen]() {
    const auto request_type = TidalRequest::FromSearchType(type);
    const int limit = StreamingSearchOpts::LimitFor(name(), type);
    TidalRequest::GetAll(
        network_,
        [this, request_type, query](int offset, int page_limit) {
          return TidalRequest::Url(kApiUrl, request_type, query, country_code_, user_id_, offset, page_limit);
        },
        AuthHeaders(), request_type,
        [this, type, guarded, gen](const SongList &songs) {
          const SongList cleaned = StreamingSearchOpts::Finish(songs, name());
          if (!StreamingSearchOpts::ShouldFetchAlbums(name(), type)) {
            guarded(cleaned);
            return;
          }
          const std::vector<std::string> ids = StreamingSearchOpts::UniqueAlbumIds(cleaned);
          if (ids.empty()) {
            guarded(cleaned);
            return;
          }
          SearchUpdateStatus.Emit(last_search_id_, StreamingProgress::RetrievingSongsForAlbums(static_cast<int>(ids.size())));
          StreamingSearchOpts::FetchEachAlbum(
              ids,
              [this, gen](const std::string &id, SearchCallback done) {
                if (!SearchRequestCurrent(gen)) {
                  done({});
                  return;
                }
                TidalRequest::GetAll(
                    network_, [this, id](int offset, int page_limit) { return TidalRequest::AlbumSongsUrl(kApiUrl, id, country_code_, offset, page_limit); },
                    AuthHeaders(), TidalRequest::Type::SearchSongs, std::move(done));
              },
              [this, guarded](const SongList &album_songs) { guarded(StreamingSearchOpts::Finish(album_songs, name())); },
              [this, gen]() { return SearchRequestCurrent(gen); },
              [this](int received, int total) { ReportSearchProgress(received, total); });
        },
        [this](int received, int total) { ReportSearchProgress(received, total); }, [this, gen]() { return SearchRequestCurrent(gen); }, limit, limit);
  });
}

void TidalService::GetArtists(SearchCallback callback) {
  auto guarded = GuardArtists(std::move(callback));
  const int gen = artists_generation();
  EnsureFreshToken([this, guarded, gen]() {
    TidalRequest::GetAll(
        network_,
        [this](int offset, int limit) {
          return TidalRequest::Url(kApiUrl, TidalRequest::Type::FavouriteArtists, {}, country_code_, user_id_, offset, limit);
        },
        AuthHeaders(), TidalRequest::Type::FavouriteArtists,
        [this, guarded](const SongList &songs) { guarded(StreamingSearchOpts::Finish(songs, name())); },
        [this](int received, int total) { ReportArtistsProgress(received, total); }, [this, gen]() { return ArtistsRequestCurrent(gen); });
  });
}

void TidalService::GetAlbums(SearchCallback callback) {
  auto guarded = GuardAlbums(std::move(callback));
  const int gen = albums_generation();
  EnsureFreshToken([this, guarded, gen]() {
    TidalRequest::GetAll(
        network_,
        [this](int offset, int limit) {
          return TidalRequest::Url(kApiUrl, TidalRequest::Type::FavouriteAlbums, {}, country_code_, user_id_, offset, limit);
        },
        AuthHeaders(), TidalRequest::Type::FavouriteAlbums,
        [this, guarded](const SongList &songs) { guarded(StreamingSearchOpts::Finish(songs, name())); },
        [this](int received, int total) { ReportAlbumsProgress(received, total); }, [this, gen]() { return AlbumsRequestCurrent(gen); });
  });
}

void TidalService::GetSongs(SearchCallback callback) {
  auto guarded = GuardSongs(std::move(callback));
  const int gen = songs_generation();
  EnsureFreshToken([this, guarded, gen]() {
    TidalRequest::GetAll(
        network_,
        [this](int offset, int limit) {
          return TidalRequest::Url(kApiUrl, TidalRequest::Type::FavouriteSongs, {}, country_code_, user_id_, offset, limit);
        },
        AuthHeaders(), TidalRequest::Type::FavouriteSongs,
        [this, guarded](const SongList &songs) { guarded(StreamingSearchOpts::Finish(songs, name())); },
        [this](int received, int total) { ReportSongsProgress(received, total); }, [this, gen]() { return SongsRequestCurrent(gen); });
  });
}

void TidalService::GetArtistAlbums(const Song &artist, SearchCallback callback) {
  const std::string id = artist.artist_id();
  if (id.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  EnsureFreshToken([this, id, callback]() {
    TidalRequest::GetAll(network_,
                         [this, id](int offset, int limit) { return TidalRequest::ArtistAlbumsUrl(kApiUrl, id, country_code_, offset, limit); },
                         AuthHeaders(), TidalRequest::Type::SearchAlbums,
                         [this, callback](const SongList &songs) {
                           if (callback) {
                             callback(StreamingSearchOpts::Finish(songs, name()));
                           }
                         });
  });
}

void TidalService::GetAlbumSongs(const Song &album, SearchCallback callback) {
  const std::string id = album.album_id();
  if (id.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  EnsureFreshToken([this, id, callback]() {
    TidalRequest::GetAll(network_,
                         [this, id](int offset, int limit) { return TidalRequest::AlbumSongsUrl(kApiUrl, id, country_code_, offset, limit); },
                         AuthHeaders(), TidalRequest::Type::SearchSongs,
                         [this, callback](const SongList &songs) {
                           if (callback) {
                             callback(StreamingSearchOpts::Finish(songs, name()));
                           }
                         });
  });
}

UrlHandler::LoadResult TidalService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  result.media_url = url;
  const std::string id = TidalStreamUrlRequest::TrackId(url);
  if (!network_ || id.empty()) {
    result.error = "Tidal is not signed in";
    if (callback) {
      callback(result);
    }
    return result;
  }
  result.type = LoadResult::Type::WillLoadAsynchronously;
  EnsureFreshToken([this, url, id, callback]() {
    if (token_.empty()) {
      LoadResult async;
      async.media_url = url;
      async.error = "Tidal is not signed in";
      async.type = LoadResult::Type::Error;
      if (callback) {
        callback(async);
      }
      return;
    }
    TidalStreamUrlRequest::Get(network_, TidalStreamUrlRequest::Url(kApiUrl, stream_url_method_, id, country_code_, quality_), AuthHeaders(),
                               url, id, callback);
  });
  return result;
}

void TidalService::GetFavorites(FavoriteType type, SearchCallback callback) {
  EnsureFreshToken([this, type, callback]() {
    TidalFavoriteRequest::Get(network_, kApiUrl, user_id_, country_code_, AuthHeaders(), type, callback,
                              [this](int received, int total) { ReportSongsProgress(received, total); });
  });
}

void TidalService::AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  EnsureFreshToken([this, type, songs, callback]() {
    TidalFavoriteRequest::Add(network_, kApiUrl, user_id_, country_code_, AuthHeaders(), type, songs, callback);
  });
}

void TidalService::RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  EnsureFreshToken([this, type, songs, callback]() {
    TidalFavoriteRequest::Remove(network_, kApiUrl, user_id_, country_code_, AuthHeaders(), type, songs, callback);
  });
}
