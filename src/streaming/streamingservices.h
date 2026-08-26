#ifndef STRAWBERRY_STREAMINGSERVICES_H
#define STRAWBERRY_STREAMINGSERVICES_H
#include "core/network.h"
#include "core/song.h"
#include "core/taskmanager.h"
#include "core/urlhandlers.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>
class StreamingService : public UrlHandler {
 public:
  using SearchCallback = std::function<void(const SongList &)>;
  enum class FavoriteType {
    Artists = 1,
    Albums = 2,
    Songs = 3
  };
  enum class SearchType {
    Artists = 1,
    Albums = 2,
    Songs = 3
  };
  virtual std::string name() const = 0;
  virtual void Search(const std::string &query, SearchCallback callback) = 0;
  virtual void Search(const std::string &query, SearchType type, SearchCallback callback) {
    if (type == SearchType::Songs) {
      Search(query, std::move(callback));
      return;
    }
    if (callback) {
      callback({});
    }
  }
  virtual void GetArtists(SearchCallback callback) {
    if (callback) {
      callback({});
    }
  }
  virtual void GetAlbums(SearchCallback callback) {
    if (callback) {
      callback({});
    }
  }
  virtual void GetSongs(SearchCallback callback) {
    if (callback) {
      callback({});
    }
  }
  virtual void Login(const std::string &username, const std::string &password_or_token) = 0;
  virtual void Logout() { logged_in_ = false; }
  virtual void ReloadSettings() {}
  virtual bool logged_in() const { return logged_in_; }
  virtual void GetFavorites(FavoriteType, SearchCallback callback) {
    if (callback) {
      callback({});
    }
  }
  virtual void AddFavorites(FavoriteType, const SongList &songs, SearchCallback callback = {}) {
    if (callback) {
      callback(songs);
    }
  }
  virtual void RemoveFavorites(FavoriteType, const SongList &songs, SearchCallback callback = {}) {
    if (callback) {
      callback(songs);
    }
  }
 protected:
  bool logged_in_ = false;
};
class StreamingServices {
 public:
  StreamingServices(NetworkAccessManager *network, UrlHandlers *url_handlers, TaskManager *task_manager = nullptr);
  std::vector<StreamingService *> All() const;
  StreamingService *ServiceByName(const std::string &name) const;
 private:
  std::vector<std::unique_ptr<StreamingService>> services_;
  std::vector<std::unique_ptr<UrlHandler>> handlers_;
};
#endif
