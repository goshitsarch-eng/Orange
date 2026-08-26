#ifndef STRAWBERRY_URLHANDLERS_H
#define STRAWBERRY_URLHANDLERS_H

#include "core/song.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

class UrlHandler {
 public:
  struct LoadResult {
    enum class Type { Error, TrackAvailable, Async, NoMoreTracks, WillLoadAsynchronously };
    Type type = Type::Error;
    std::string media_url;
    std::string stream_url;
    std::string error;
    Song song;
  };

  using AsyncCallback = std::function<void(const LoadResult &)>;

  virtual ~UrlHandler() = default;
  virtual std::string scheme() const = 0;
  virtual LoadResult Load(const std::string &url, AsyncCallback callback = {}) = 0;
};

class UrlHandlers {
 public:
  void AddHandler(UrlHandler *handler);
  UrlHandler *HandlerForUrl(const std::string &url) const;
  std::vector<std::string> Schemes() const;

 private:
  std::map<std::string, UrlHandler *> handlers_;
};

#endif
