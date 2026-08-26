#ifndef STRAWBERRY_SPOTIFYSERVICE_H
#define STRAWBERRY_SPOTIFYSERVICE_H

#include "streaming/streamingservices.h"

#include <map>
#include <string>

class SpotifyService : public StreamingService {
 public:
  static const char kApiUrl[];

  explicit SpotifyService(NetworkAccessManager *network);

  std::string name() const override { return "Spotify"; }
  std::string scheme() const override { return "spotify"; }
  void Search(const std::string &query, SearchCallback callback) override;
  void Login(const std::string &username, const std::string &password_or_token) override;
  void ReloadSettings() override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;
  void GetFavorites(FavoriteType type, SearchCallback callback) override;
  void AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;
  void RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;

 private:
  std::map<std::string, std::string> AuthHeaders() const;

  NetworkAccessManager *network_ = nullptr;
  std::string token_;
};

#endif
