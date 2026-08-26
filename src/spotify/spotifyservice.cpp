#include "spotify/spotifyservice.h"

#include "constants/spotifysettings.h"
#include "core/settings.h"
#include "spotify/spotifyfavoriterequest.h"
#include "spotify/spotifymetadatarequest.h"
#include "spotify/spotifyrequest.h"
#include "streaming/streamingmediaid.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <ctime>

const char SpotifyService::kApiUrl[] = "https://api.spotify.com/v1";

SpotifyService::SpotifyService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

void SpotifyService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup(SpotifySettings::kSettingsGroup);
  token_ = settings.Value("token");
  if (token_.empty()) {
    token_ = settings.Value(SpotifySettings::kAccessToken);
  }
  refresh_token_ = settings.Value(SpotifySettings::kRefreshToken);
  expires_in_ = settings.IntValue(SpotifySettings::kExpiresIn);
  login_time_ = settings.Int64Value(SpotifySettings::kLoginTime);
  client_id_ = settings.Value("clientid");
  client_secret_ = settings.Value("clientsecret");
  logged_in_ = !token_.empty();
}

void SpotifyService::StoreTokens(const OAuthenticator::TokenResponse &tokens) {
  Settings settings;
  settings.BeginGroup(SpotifySettings::kSettingsGroup);
  if (!tokens.access_token.empty()) {
    settings.SetValue("token", tokens.access_token);
    settings.SetValue(SpotifySettings::kAccessToken, tokens.access_token);
  }
  if (!tokens.refresh_token.empty()) {
    settings.SetValue(SpotifySettings::kRefreshToken, tokens.refresh_token);
  }
  if (tokens.expires_in > 0) {
    settings.SetIntValue(SpotifySettings::kExpiresIn, tokens.expires_in);
    settings.SetInt64Value(SpotifySettings::kLoginTime, static_cast<gint64>(std::time(nullptr)));
  }
  settings.Sync();
  ReloadSettings();
}

void SpotifyService::Login(const std::string &username, const std::string &password_or_token) {
  Settings settings;
  settings.BeginGroup(SpotifySettings::kSettingsGroup);
  if (!username.empty()) {
    settings.SetValue("username", username);
  }
  settings.SetValue("token", password_or_token);
  settings.SetValue(SpotifySettings::kAccessToken, password_or_token);
  settings.Sync();
  ReloadSettings();
}

void SpotifyService::EnsureFreshToken(std::function<void()> next) {
  if (refresh_token_.empty() || !OAuthenticator::AccessTokenExpired(login_time_, expires_in_)) {
    if (next) {
      next();
    }
    return;
  }
  auto *oauth = new OAuthenticator(network_);
  oauth->RefreshAccessToken("https://accounts.spotify.com/api/token", client_id_, client_secret_, refresh_token_,
                            [this, oauth, next](const std::string &body, const std::string &error) {
                              if (error.empty()) {
                                StoreTokens(OAuthenticator::ParseTokenResponse(body));
                              }
                              delete oauth;
                              if (next) {
                                next();
                              }
                            });
}

std::map<std::string, std::string> SpotifyService::AuthHeaders() const {
  if (token_.empty()) {
    return {};
  }
  return {{"Authorization", "Bearer " + token_}};
}

void SpotifyService::Search(const std::string &query, SearchCallback callback) { Search(query, SearchType::Songs, std::move(callback)); }

void SpotifyService::Search(const std::string &query, SearchType type, SearchCallback callback) {
  EnsureFreshToken([this, query, type, callback]() {
    const auto request_type = SpotifyRequest::FromSearchType(type);
    SpotifyRequest::Get(network_, SpotifyRequest::Url(kApiUrl, request_type, query), AuthHeaders(), request_type, callback);
  });
}

void SpotifyService::GetArtists(SearchCallback callback) {
  EnsureFreshToken([this, callback]() {
    SpotifyRequest::Get(network_, SpotifyRequest::Url(kApiUrl, SpotifyRequest::Type::FavouriteArtists, {}), AuthHeaders(),
                        SpotifyRequest::Type::FavouriteArtists, callback);
  });
}

void SpotifyService::GetAlbums(SearchCallback callback) {
  EnsureFreshToken([this, callback]() {
    SpotifyRequest::Get(network_, SpotifyRequest::Url(kApiUrl, SpotifyRequest::Type::FavouriteAlbums, {}), AuthHeaders(),
                        SpotifyRequest::Type::FavouriteAlbums, callback);
  });
}

void SpotifyService::GetSongs(SearchCallback callback) {
  EnsureFreshToken([this, callback]() {
    SpotifyRequest::Get(network_, SpotifyRequest::Url(kApiUrl, SpotifyRequest::Type::FavouriteSongs, {}), AuthHeaders(),
                        SpotifyRequest::Type::FavouriteSongs, callback);
  });
}

UrlHandler::LoadResult SpotifyService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  result.media_url = url;
  const std::string id = StreamingMediaId(url);
  if (!network_ || id.empty()) {
    result.error = "Spotify is not signed in";
    if (callback) {
      callback(result);
    }
    return result;
  }
  result.type = LoadResult::Type::WillLoadAsynchronously;
  EnsureFreshToken([this, callback, url, id]() {
    if (token_.empty()) {
      LoadResult async;
      async.media_url = url;
      async.error = "Spotify is not signed in";
      async.type = LoadResult::Type::Error;
      if (callback) {
        callback(async);
      }
      return;
    }
    const auto headers = AuthHeaders();
    SpotifyMetadataRequest::Get(network_, SpotifyMetadataRequest::TrackUrl(kApiUrl, id), headers,
                              [this, callback, url, headers](const Song &song, const std::string &error) {
                                LoadResult async;
                                async.media_url = url;
                                async.song = song;
                                async.stream_url = song.stream_url() == song.url() ? std::string() : song.stream_url();
                                if (async.stream_url.empty()) {
                                  async.error = error.empty() ? "Spotify preview URL missing" : error;
                                  async.type = LoadResult::Type::Error;
                                  if (callback) {
                                    callback(async);
                                  }
                                  return;
                                }
                                async.duration = song.length_nanosec() > 0 ? song.length_nanosec() : -1;
                                async.type = LoadResult::Type::TrackAvailable;
                                if (song.artist_id().empty() || !network_) {
                                  if (callback) {
                                    callback(async);
                                  }
                                  return;
                                }
                                network_->Get(
                                    SpotifyMetadataRequest::ArtistUrl(kApiUrl, song.artist_id()),
                                    [callback, async](const NetworkAccessManager::Response &response) mutable {
                                      if (response.ok()) {
                                        const std::string genre = SpotifyMetadataRequest::ParseArtistGenre(response.body);
                                        if (!genre.empty()) {
                                          async.song.set_genre(genre);
                                        }
                                      }
                                      if (callback) {
                                        callback(async);
                                      }
                                    },
                                    headers);
                              });
  });
  return result;
}

void SpotifyService::GetFavorites(FavoriteType type, SearchCallback callback) {
  EnsureFreshToken([this, type, callback]() { SpotifyFavoriteRequest::Get(network_, kApiUrl, AuthHeaders(), type, callback); });
}

void SpotifyService::AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  EnsureFreshToken([this, type, songs, callback]() { SpotifyFavoriteRequest::Add(network_, kApiUrl, AuthHeaders(), type, songs, callback); });
}

void SpotifyService::RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  EnsureFreshToken([this, type, songs, callback]() { SpotifyFavoriteRequest::Remove(network_, kApiUrl, AuthHeaders(), type, songs, callback); });
}
