#ifndef STRAWBERRY_AUDIOSCROBBLER_H
#define STRAWBERRY_AUDIOSCROBBLER_H
#include "core/network.h"
#include "core/song.h"
#include "core/signal.h"
#include <memory>
#include <string>
#include <vector>
class ScrobblerService {
 public:
  virtual ~ScrobblerService() = default;
  virtual std::string name() const = 0;
  virtual bool enabled() const { return enabled_; }
  virtual void set_enabled(bool e) { enabled_ = e; }
  virtual void NowPlaying(const Song &song) = 0;
  virtual void Scrobble(const Song &song) = 0;
  virtual void Love(const Song &song) = 0;
  virtual void Authenticate(const std::string &username, const std::string &password) = 0;
 protected:
  bool enabled_ = false;
};
class AudioScrobbler {
 public:
  explicit AudioScrobbler(NetworkAccessManager *network);
  void ReloadSettings();
  void NowPlaying(const Song &song);
  void Scrobble(const Song &song);
  void Love(const Song &song);
  std::vector<ScrobblerService *> All() const;
  Signal<std::string> Error;
 private:
  NetworkAccessManager *network_;
  std::vector<std::unique_ptr<ScrobblerService>> services_;
};
#endif
