#ifndef STRAWBERRY_QOBUZSERVICE_H
#define STRAWBERRY_QOBUZSERVICE_H

#include "streaming/streamingservices.h"

#include <functional>
#include <map>
#include <string>

class QobuzService : public StreamingService {
 public:
  static const char kApiUrl[];

  explicit QobuzService(NetworkAccessManager *network);

  std::string name() const override { return "Qobuz"; }
  std::string scheme() const override { return "qobuz"; }
  NetworkAccessManager *network() const override { return network_; }
  void Search(const std::string &query, SearchCallback callback) override;
  void Search(const std::string &query, SearchType type, SearchCallback callback) override;
  void GetArtists(SearchCallback callback) override;
  void GetAlbums(SearchCallback callback) override;
  void GetSongs(SearchCallback callback) override;
  void GetArtistAlbums(const Song &artist, SearchCallback callback) override;
  void GetAlbumSongs(const Song &album, SearchCallback callback) override;
  void Login(const std::string &username, const std::string &password_or_token) override;
  void Logout() override;
  void ReloadSettings() override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;
  void FetchTrackMetadata(const std::string &track_id, std::function<void(const Song &, const std::string &error)> callback);
  void GetFavorites(FavoriteType type, SearchCallback callback) override;
  void AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;
  void RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;

  const std::string &app_id() const { return app_id_; }
  const std::string &app_secret() const { return app_secret_; }
  const std::string &user_auth_token() const { return user_auth_token_; }
  int format() const { return format_; }

 private:
  std::map<std::string, std::string> AuthHeaders() const;

  NetworkAccessManager *network_ = nullptr;
  std::string app_id_;
  std::string app_secret_;
  std::string user_auth_token_;
  int format_ = 27;
};

#endif
