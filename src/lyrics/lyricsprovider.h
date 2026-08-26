#ifndef STRAWBERRY_LYRICSPROVIDER_H
#define STRAWBERRY_LYRICSPROVIDER_H

#include "core/network.h"
#include "core/song.h"
#include "lyrics/lyricssearchrequest.h"
#include "lyrics/lyricssearchresult.h"

#include <functional>
#include <string>

class LyricsProvider {
 public:
  using Callback = std::function<void(const std::string &lyrics, const std::string &error)>;
  virtual ~LyricsProvider() = default;
  virtual std::string name() const = 0;
  virtual bool enabled() const { return enabled_; }
  virtual void set_enabled(bool enabled) { enabled_ = enabled; }
  virtual int order() const { return order_; }
  virtual void set_order(int order) { order_ = order; }
  virtual void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) = 0;
  virtual bool StartSearch(int id, const LyricsSearchRequest &request, NetworkAccessManager *network,
                           const std::function<void(int, const LyricsSearchResults &)> &finished);

 protected:
  bool enabled_ = true;
  int order_ = 0;
};

#endif
