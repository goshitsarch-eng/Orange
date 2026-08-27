#ifndef STRAWBERRY_SCROBBLERSERVICE_H
#define STRAWBERRY_SCROBBLERSERVICE_H

#include "core/signal.h"
#include "core/song.h"

#include <string>

class ScrobblerService {
 public:
  virtual ~ScrobblerService() = default;
  virtual std::string name() const = 0;
  virtual bool enabled() const { return enabled_; }
  virtual void set_enabled(bool e) { enabled_ = e; }
  virtual void NowPlaying(const Song &song) = 0;
  virtual void ClearPlaying() {}
  virtual void Scrobble(const Song &song) = 0;
  virtual void Love(const Song &song) = 0;
  virtual void Authenticate(const std::string &username, const std::string &password) = 0;
  virtual void StartAuthentication() {}
  virtual void Logout() {}
  virtual bool authenticated() const { return false; }
  virtual std::string username() const { return {}; }
  virtual void WriteCache() {}
  virtual void Submit() {}

  Signal<std::string> Error;

 protected:
  bool enabled_ = false;
};

#endif
