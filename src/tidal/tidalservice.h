#ifndef STRAWBERRY_TIDALSERVICE_H
#define STRAWBERRY_TIDALSERVICE_H

#include "constants/tidalsettings.h"
#include "streaming/streamingservices.h"

#include <cstdint>
#include <map>
#include <string>

class TidalService : public StreamingService {
 public:
  static const char kApiUrl[];
  static const char kResourcesUrl[];

  explicit TidalService(NetworkAccessManager *network);

  std::string name() const override { return "Tidal"; }
  std::string scheme() const override { return "tidal"; }
  const std::string &quality() const { return quality_; }
  TidalSettings::StreamUrlMethod stream_url_method() const { return stream_url_method_; }
  void Search(const std::string &query, SearchCallback callback) override;
  void Search(const std::string &query, SearchType type, SearchCallback callback) override;
  void GetArtists(SearchCallback callback) override;
  void GetAlbums(SearchCallback callback) override;
  void GetSongs(SearchCallback callback) override;
  void Login(const std::string &username, const std::string &password_or_token) override;
  void ReloadSettings() override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;
  void GetFavorites(FavoriteType type, SearchCallback callback) override;
  void AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;
  void RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;

  uint64_t user_id() const { return user_id_; }
  const std::string &country_code() const { return country_code_; }

 private:
  std::map<std::string, std::string> AuthHeaders() const;

  NetworkAccessManager *network_ = nullptr;
  std::string token_;
  std::string country_code_ = "US";
  std::string quality_ = TidalSettings::kDefaultQuality;
  TidalSettings::StreamUrlMethod stream_url_method_ = TidalSettings::kDefaultStreamUrl;
  uint64_t user_id_ = 0;
};

#endif
