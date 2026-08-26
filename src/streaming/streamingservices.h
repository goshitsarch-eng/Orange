#ifndef STRAWBERRY_STREAMINGSERVICES_H
#define STRAWBERRY_STREAMINGSERVICES_H
#include "core/network.h"
#include "core/song.h"
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
  virtual std::string name() const = 0;
  virtual void Search(const std::string &query, SearchCallback callback) = 0;
  virtual void Login(const std::string &username, const std::string &password_or_token) = 0;
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
  StreamingServices(NetworkAccessManager *network, UrlHandlers *url_handlers);
  std::vector<StreamingService *> All() const;
  StreamingService *ServiceByName(const std::string &name) const;
 private:
  std::vector<std::unique_ptr<StreamingService>> services_;
};
#endif
