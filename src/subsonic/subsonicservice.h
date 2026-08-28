#ifndef STRAWBERRY_SUBSONICSERVICE_H
#define STRAWBERRY_SUBSONICSERVICE_H

#include "streaming/streamingservices.h"

#include <map>
#include <string>

class SubsonicService : public StreamingService {
 public:
  static const char *kClientName;
  static const char *kApiVersion;

  explicit SubsonicService(NetworkAccessManager *network);

  std::string name() const override { return "Subsonic"; }
  std::string scheme() const override { return "subsonic"; }
  NetworkAccessManager *network() const override { return network_; }
  void Search(const std::string &query, SearchCallback callback) override;
  void Search(const std::string &query, SearchType type, SearchCallback callback) override;
  void GetArtists(SearchCallback callback) override;
  void GetAlbums(SearchCallback callback) override;
  void GetSongs(SearchCallback callback) override;
  void GetArtistAlbums(const Song &artist, SearchCallback callback) override;
  void GetAlbumSongs(const Song &album, SearchCallback callback) override;
  void Login(const std::string &username, const std::string &password_or_token) override;
  void ReloadSettings() override;
  LoadResult Load(const std::string &url, AsyncCallback callback = {}) override;
  void GetFavorites(FavoriteType type, SearchCallback callback) override;
  void AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;
  void RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {}) override;

  static std::string CreateUrl(const std::string &server_url, const std::string &username, const std::string &password,
                               const std::string &resource, const std::map<std::string, std::string> &params = {}, bool hex_auth = false);
  static std::string Md5Hex(const std::string &value);
  static std::string HexEncode(const std::string &value);
  static std::string RandomSalt(int length = 20);
  SongList WithCoverUrls(SongList songs) const override;

 private:
  NetworkAccessManager *network_ = nullptr;
  std::string server_url_;
  std::string username_;
  std::string password_;
  static std::string SubsonicHost(const std::string &url);

  bool hex_auth_ = false;
  bool use_album_id_for_covers_ = false;
};

#endif
