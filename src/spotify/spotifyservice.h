#ifndef STRAWBERRY_SPOTIFYSERVICE_H
#define STRAWBERRY_SPOTIFYSERVICE_H

#include "core/oauthenticator.h"
#include "streaming/streamingservices.h"

#include <functional>
#include <map>
#include <string>

class SpotifyService : public StreamingService {
 public:
  static const char kApiUrl[];

  explicit SpotifyService(NetworkAccessManager *network);

  std::string name() const override { return "Spotify"; }
  std::string scheme() const override { return "spotify"; }
  void Search(const std::string &query, SearchCallback callback) override;
  void Search(const std::string &query, SearchType type, SearchCallback callback) override;
  void GetArtists(SearchCallback callback) override;
  void GetAlbums(SearchCallback callback) override;
  void GetSongs(SearchCallback callback) override;
  void Login(const std::string &username, const std::string &password_or_token) override;
  void Logout() override;
  void StoreTokens(const OAuthenticator::TokenResponse &tokens);
  void ReloadSettings() override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;
  void GetFavorites(FavoriteType type, SearchCallback callback) override;
  void AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;
  void RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;

 private:
  std::map<std::string, std::string> AuthHeaders() const;
  void EnsureFreshToken(std::function<void()> next);

  NetworkAccessManager *network_ = nullptr;
  std::string token_;
  std::string refresh_token_;
  std::string client_id_;
  std::string client_secret_;
  int expires_in_ = 0;
  gint64 login_time_ = 0;
};

#endif
