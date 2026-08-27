#ifndef STRAWBERRY_URLHANDLER_H
#define STRAWBERRY_URLHANDLER_H

#include "core/song.h"

#include <cstdint>
#include <functional>
#include <string>

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

#endif
