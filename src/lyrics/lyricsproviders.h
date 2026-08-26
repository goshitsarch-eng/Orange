#ifndef STRAWBERRY_LYRICSPROVIDERS_H
#define STRAWBERRY_LYRICSPROVIDERS_H

#include "core/network.h"
#include "core/song.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class LyricsProvider {
 public:
  using Callback = std::function<void(const std::string &lyrics, const std::string &error)>;
  virtual ~LyricsProvider() = default;
  virtual std::string name() const = 0;
  virtual bool enabled() const { return enabled_; }
  virtual void set_enabled(bool enabled) { enabled_ = enabled; }
  virtual void Fetch(const Song &song, NetworkAccessManager *network, Callback callback) = 0;
 protected:
  bool enabled_ = true;
};

class LyricsProviders {
 public:
  explicit LyricsProviders(NetworkAccessManager *network);
  void ReloadSettings();
  void Fetch(const Song &song, LyricsProvider::Callback callback);
  std::vector<LyricsProvider *> All() const;

 private:
  void FetchFromIndex(const Song &song, size_t index, LyricsProvider::Callback callback);

  NetworkAccessManager *network_;
  std::vector<std::unique_ptr<LyricsProvider>> providers_;
};
#endif
