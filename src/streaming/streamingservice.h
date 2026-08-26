#ifndef STRAWBERRY_STREAMINGSERVICE_H
#define STRAWBERRY_STREAMINGSERVICE_H

#include "core/signal.h"
#include "core/song.h"
#include "core/urlhandler.h"

#include <functional>
#include <string>

class NetworkAccessManager;

class StreamingService : public UrlHandler {
 public:
  using SearchCallback = std::function<void(const SongList &)>;
  enum class FavoriteType { Artists = 1, Albums = 2, Songs = 3 };
  enum class SearchType { Artists = 1, Albums = 2, Songs = 3 };

  virtual std::string name() const = 0;
  virtual NetworkAccessManager *network() const { return nullptr; }
  virtual void Search(const std::string &query, SearchCallback callback) = 0;
  virtual void Search(const std::string &query, SearchType type, SearchCallback callback);
  virtual void GetArtists(SearchCallback callback);
  virtual void GetAlbums(SearchCallback callback);
  virtual void GetSongs(SearchCallback callback);
  virtual void GetArtistAlbums(const Song &artist, SearchCallback callback);
  virtual void GetAlbumSongs(const Song &album, SearchCallback callback);
  virtual void Login(const std::string &username, const std::string &password_or_token) = 0;
  virtual void Logout();
  virtual void ReloadSettings() {}
  virtual bool logged_in() const { return logged_in_; }
  virtual bool authenticated() const { return logged_in_; }
  void NotifyAuthenticationChanged();
  void NotifyAuthenticationFailed(const std::string &error);
  int last_search_id() const { return last_search_id_; }
  int StartSearchProgress();
  virtual void GetFavorites(FavoriteType type, SearchCallback callback);
  virtual void AddFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {});
  virtual void RemoveFavorites(FavoriteType type, const SongList &songs, SearchCallback callback = {});

  Signal<> AuthenticationChanged;
  Signal<std::string> AuthenticationFailed;
  Signal<int, std::string> SearchUpdateStatus;
  Signal<int, int> SearchProgressSetMaximum;
  Signal<int, int> SearchUpdateProgress;

 protected:
  bool logged_in_ = false;
  int last_search_id_ = 0;
};

#endif
