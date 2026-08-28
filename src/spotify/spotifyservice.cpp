#include "spotify/spotifyservice.h"

#include "constants/spotifysettings.h"
#include "core/settings.h"
#include "streaming/streamingalbum.h"
#include "streaming/streamingauth.h"
#include "streaming/streamingprogress.h"
#include "streaming/streamingsearchopts.h"
#include "spotify/spotifycredentials.h"
#include "spotify/spotifyfavoriterequest.h"
#include "spotify/spotifymetadatarequest.h"
#include "spotify/spotifyplayback.h"
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
    token_ = settings.SecretValue(SpotifySettings::kAccessToken);
  }
  refresh_token_ = settings.SecretValue(SpotifySettings::kRefreshToken);
  expires_in_ = settings.IntValue(SpotifySettings::kExpiresIn);
  login_time_ = settings.Int64Value(SpotifySettings::kLoginTime);
  client_id_ = SpotifyCredentials::EffectiveClientId(settings.Value("clientid"));
  client_secret_ = SpotifyCredentials::EffectiveClientSecret(settings.Value("clientsecret"));
  logged_in_ = !token_.empty();
}

void SpotifyService::StoreTokens(const OAuthenticator::TokenResponse &tokens) {
  Settings settings;
  settings.BeginGroup(SpotifySettings::kSettingsGroup);
  if (!tokens.access_token.empty()) {
    settings.SetValue("token", tokens.access_token);
    settings.SetSecretValue(SpotifySettings::kAccessToken, tokens.access_token);
  }
  if (!tokens.refresh_token.empty()) {
    settings.SetSecretValue(SpotifySettings::kRefreshToken, tokens.refresh_token);
  }
  if (tokens.expires_in > 0) {
    settings.SetIntValue(SpotifySettings::kExpiresIn, tokens.expires_in);
    settings.SetInt64Value(SpotifySettings::kLoginTime, static_cast<gint64>(std::time(nullptr)));
  }
  settings.Sync();
  ReloadSettings();
  NotifyAuthenticationChanged();
}

void SpotifyService::Logout() {
  StreamingAuth::ClearKeys(SpotifySettings::kSettingsGroup,
                           {"token", SpotifySettings::kAccessToken, SpotifySettings::kRefreshToken, SpotifySettings::kExpiresIn,
                            SpotifySettings::kLoginTime});
  token_.clear();
  refresh_token_.clear();
  expires_in_ = 0;
  login_time_ = 0;
  StreamingService::Logout();
}

void SpotifyService::Login(const std::string &username, const std::string &password_or_token) {
  Settings settings;
  settings.BeginGroup(SpotifySettings::kSettingsGroup);
  if (!username.empty()) {
    settings.SetValue("username", username);
  }
  settings.SetValue("token", password_or_token);
  settings.SetSecretValue(SpotifySettings::kAccessToken, password_or_token);
  settings.Sync();
  ReloadSettings();
  NotifyAuthenticationChanged();
}

void SpotifyService::EnsureFreshToken(std::function<void()> next) {
  if (StreamingAuth::EnsureAction(login_time_, expires_in_, refresh_token_) == StreamingAuth::Action::Proceed) {
    if (next) {
      next();
    }
    return;
  }
  auto *oauth = new OAuthenticator(network_);
  oauth->RefreshAccessToken("https://accounts.spotify.com/api/token", client_id_, client_secret_, refresh_token_,
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

std::map<std::string, std::string> SpotifyService::AuthHeaders() const {
  if (token_.empty()) {
    return {};
  }
  return {{"Authorization", "Bearer " + token_}};
}

void SpotifyService::Search(const std::string &query, SearchCallback callback) { Search(query, SearchType::Songs, std::move(callback)); }

void SpotifyService::Search(const std::string &query, SearchType type, SearchCallback callback) {
  auto guarded = GuardSearch(std::move(callback));
  const int gen = search_generation();
  EnsureFreshToken([this, query, type, guarded, gen]() {
    const auto request_type = SpotifyRequest::FromSearchType(type);
    const int limit = StreamingSearchOpts::LimitFor(name(), type);
    SpotifyRequest::GetAll(
        network_, [request_type, query](int offset, int page_limit) { return SpotifyRequest::Url(SpotifyService::kApiUrl, request_type, query, offset, page_limit); },
        AuthHeaders(), request_type,
        [this, type, guarded, gen](const SongList &songs) {
          const SongList cleaned = StreamingSearchOpts::Finish(songs, name());
          auto deliver = [this, guarded, gen](const SongList &ready) {
            DeliverWithCovers(network_, AuthHeaders(), ready, guarded,
                              [this](const std::string &text) { SearchUpdateStatus.Emit(last_search_id_, text); },
                              [this](int received, int total) { ReportSearchProgress(received, total); },
                              [this, gen]() { return SearchRequestCurrent(gen); });
          };
          if (!StreamingSearchOpts::ShouldFetchAlbums(name(), type)) {
            deliver(cleaned);
            return;
          }
          const std::vector<std::string> ids = StreamingSearchOpts::UniqueAlbumIds(cleaned);
          if (ids.empty()) {
            deliver(cleaned);
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
                SpotifyRequest::GetAll(network_,
                                       [id](int offset, int page_limit) { return SpotifyRequest::AlbumSongsUrl(SpotifyService::kApiUrl, id, offset, page_limit); },
                                       AuthHeaders(), SpotifyRequest::Type::SearchSongs, std::move(done));
              },
              deliver, [this, gen]() { return SearchRequestCurrent(gen); },
              [this](int received, int total) { ReportSearchProgress(received, total); });
        },
        [this](int received, int total) { ReportSearchProgress(received, total); }, [this, gen]() { return SearchRequestCurrent(gen); }, limit, limit,
        [this](const std::string &error) { NotifySearchFailed(error); });
  });
}

void SpotifyService::GetArtists(SearchCallback callback) {
  auto guarded = GuardArtists(std::move(callback));
  const int gen = artists_generation();
  EnsureFreshToken([this, guarded, gen]() {
    SpotifyRequest::GetAll(
        network_,
        [](int offset, int limit) { return SpotifyRequest::Url(SpotifyService::kApiUrl, SpotifyRequest::Type::FavouriteArtists, {}, offset, limit); },
        AuthHeaders(), SpotifyRequest::Type::FavouriteArtists,
        [this, guarded, gen](const SongList &songs) {
          DeliverWithCovers(network_, AuthHeaders(), songs, guarded, [this](const std::string &text) { ArtistsUpdateStatus.Emit(text); },
                            [this](int received, int total) { ReportArtistsProgress(received, total); },
                            [this, gen]() { return ArtistsRequestCurrent(gen); });
        },
        [this](int received, int total) { ReportArtistsProgress(received, total); }, [this, gen]() { return ArtistsRequestCurrent(gen); },
        StreamingPage::kDefaultLimit, 0, [this](const std::string &error) { NotifyArtistsFailed(error); });
  });
}

void SpotifyService::GetAlbums(SearchCallback callback) {
  auto guarded = GuardAlbums(std::move(callback));
  const int gen = albums_generation();
  EnsureFreshToken([this, guarded, gen]() {
    SpotifyRequest::GetAll(
        network_,
        [](int offset, int limit) { return SpotifyRequest::Url(SpotifyService::kApiUrl, SpotifyRequest::Type::FavouriteAlbums, {}, offset, limit); },
        AuthHeaders(), SpotifyRequest::Type::FavouriteAlbums,
        [this, guarded, gen](const SongList &songs) {
          DeliverWithCovers(network_, AuthHeaders(), songs, guarded, [this](const std::string &text) { AlbumsUpdateStatus.Emit(text); },
                            [this](int received, int total) { ReportAlbumsProgress(received, total); },
                            [this, gen]() { return AlbumsRequestCurrent(gen); });
        },
        [this](int received, int total) { ReportAlbumsProgress(received, total); }, [this, gen]() { return AlbumsRequestCurrent(gen); },
        StreamingPage::kDefaultLimit, 0, [this](const std::string &error) { NotifyAlbumsFailed(error); });
  });
}

void SpotifyService::GetSongs(SearchCallback callback) {
  auto guarded = GuardSongs(std::move(callback));
  const int gen = songs_generation();
  EnsureFreshToken([this, guarded, gen]() {
    SpotifyRequest::GetAll(
        network_,
        [](int offset, int limit) { return SpotifyRequest::Url(SpotifyService::kApiUrl, SpotifyRequest::Type::FavouriteSongs, {}, offset, limit); },
        AuthHeaders(), SpotifyRequest::Type::FavouriteSongs,
        [this, guarded, gen](const SongList &songs) {
          DeliverWithCovers(network_, AuthHeaders(), songs, guarded, [this](const std::string &text) { SongsUpdateStatus.Emit(text); },
                            [this](int received, int total) { ReportSongsProgress(received, total); },
                            [this, gen]() { return SongsRequestCurrent(gen); });
        },
        [this](int received, int total) { ReportSongsProgress(received, total); }, [this, gen]() { return SongsRequestCurrent(gen); },
        StreamingPage::kDefaultLimit, 0, [this](const std::string &error) { NotifySongsFailed(error); });
  });
}

void SpotifyService::GetArtistAlbums(const Song &artist, SearchCallback callback) {
  const std::string id = artist.artist_id();
  if (id.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  EnsureFreshToken([this, id, callback]() {
    SpotifyRequest::GetAll(network_, [id](int offset, int limit) { return SpotifyRequest::ArtistAlbumsUrl(SpotifyService::kApiUrl, id, offset, limit); },
                           AuthHeaders(), SpotifyRequest::Type::SearchAlbums,
                           [this, callback](const SongList &songs) { DeliverWithCovers(network_, AuthHeaders(), songs, callback); });
  });
}

void SpotifyService::GetAlbumSongs(const Song &album, SearchCallback callback) {
  const std::string id = album.album_id();
  if (id.empty()) {
    if (callback) {
      callback({});
    }
    return;
  }
  EnsureFreshToken([this, id, album, callback]() {
    SpotifyRequest::GetAll(network_, [id](int offset, int limit) { return SpotifyRequest::AlbumSongsUrl(SpotifyService::kApiUrl, id, offset, limit); },
                           AuthHeaders(), SpotifyRequest::Type::SearchSongs, [this, album, callback](const SongList &songs) {
                             SongList copy = songs;
                             StreamingAlbum::ApplyParent(copy, album);
                             DeliverWithCovers(network_, AuthHeaders(), copy, callback);
                           });
  });
}

UrlHandler::LoadResult SpotifyService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  result.media_url = url;
  std::string id = SpotifyPlayback::TrackId(url);
  if (id.empty()) {
    id = StreamingMediaId(url);
  }
  if (SpotifyPlayback::UseNativePlayback(url, SpotifyPlayback::PluginAvailable(), !token_.empty()) &&
      StreamingAuth::EnsureAction(login_time_, expires_in_, refresh_token_) == StreamingAuth::Action::Proceed) {
    result = SpotifyPlayback::NativeResult(url);
    if (callback) {
      callback(result);
    }
    return result;
  }
  if (!network_ || id.empty()) {
    result.error = token_.empty() ? "Spotify is not signed in" : "Spotify track URL is missing";
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
    if (SpotifyPlayback::UseNativePlayback(url, SpotifyPlayback::PluginAvailable(), true)) {
      if (callback) {
        callback(SpotifyPlayback::NativeResult(url));
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

void SpotifyService::FetchTrackMetadata(const std::string &track_id, std::function<void(const Song &, const std::string &error)> callback) {
  if (track_id.empty()) {
    if (callback) {
      callback(Song(), "No track ID");
    }
    return;
  }
  EnsureFreshToken([this, track_id, callback]() {
    if (token_.empty()) {
      if (callback) {
        callback(Song(), "Not authenticated");
      }
      return;
    }
    const auto headers = AuthHeaders();
    SpotifyMetadataRequest::Get(network_, SpotifyMetadataRequest::TrackUrl(kApiUrl, track_id), headers,
                                [this, callback, headers](const Song &song, const std::string &error) {
                                  if (!song.is_valid()) {
                                    if (callback) {
                                      callback(song, error.empty() ? "Spotify metadata missing" : error);
                                    }
                                    return;
                                  }
                                  if (song.artist_id().empty() || !network_) {
                                    if (callback) {
                                      callback(song, {});
                                    }
                                    return;
                                  }
                                  network_->Get(
                                      SpotifyMetadataRequest::ArtistUrl(kApiUrl, song.artist_id()),
                                      [callback, song](const NetworkAccessManager::Response &response) {
                                        Song out = song;
                                        if (response.ok()) {
                                          const std::string genre = SpotifyMetadataRequest::ParseArtistGenre(response.body);
                                          if (!genre.empty()) {
                                            out.set_genre(genre);
                                          }
                                        }
                                        if (callback) {
                                          callback(out, {});
                                        }
                                      },
                                      headers);
                                });
  });
}

void SpotifyService::GetFavorites(FavoriteType type, SearchCallback callback) {
  auto guarded = GuardFavorites(std::move(callback));
  const int gen = favorites_generation();
  EnsureFreshToken([this, type, guarded, gen]() {
    SpotifyFavoriteRequest::Get(
        network_, kApiUrl, AuthHeaders(), type,
        [this, guarded, gen](const SongList &songs) {
          DeliverWithCovers(network_, AuthHeaders(), songs, guarded, [this](const std::string &text) { FavoritesUpdateStatus.Emit(text); },
                            [this](int received, int total) { ReportFavoritesProgress(received, total); },
                            [this, gen]() { return FavoritesRequestCurrent(gen); });
        },
        [this](int received, int total) { ReportFavoritesProgress(received, total); }, [this, gen]() { return FavoritesRequestCurrent(gen); },
        [this](const std::string &error) { NotifyFavoritesFailed(error); });
  });
}

void SpotifyService::AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  EnsureFreshToken([this, type, songs, callback]() { SpotifyFavoriteRequest::Add(network_, kApiUrl, AuthHeaders(), type, songs, callback); });
}

void SpotifyService::RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback) {
  EnsureFreshToken([this, type, songs, callback]() { SpotifyFavoriteRequest::Remove(network_, kApiUrl, AuthHeaders(), type, songs, callback); });
}
