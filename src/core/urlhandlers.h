#ifndef STRAWBERRY_URLHANDLERS_H
#define STRAWBERRY_URLHANDLERS_H

#include "core/song.h"

#include <cstdint>
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
    Song::FileType filetype = Song::FileType::Unknown;
    int samplerate = -1;
    int bit_depth = -1;
    int64_t duration = -1;
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
