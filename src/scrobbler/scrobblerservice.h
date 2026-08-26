#ifndef STRAWBERRY_SCROBBLERSERVICE_H
#define STRAWBERRY_SCROBBLERSERVICE_H

#include "core/song.h"

#include <string>

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

#endif
