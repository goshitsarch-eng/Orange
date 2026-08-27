#ifndef STRAWBERRY_AUDIOSCROBBLER_H
#define STRAWBERRY_AUDIOSCROBBLER_H

#include "core/network.h"
#include "core/signal.h"
#include "core/song.h"
#include "scrobbler/scrobblerservice.h"

#include <memory>
#include <string>
#include <vector>

class AudioScrobbler {
 public:
  explicit AudioScrobbler(NetworkAccessManager *network);
  void ReloadSettings();
  bool enabled() const;
  void NowPlaying(const Song &song);
  void ClearPlaying();
  void Scrobble(const Song &song);
  void Love(const Song &song);
  void ToggleScrobbling();
  ScrobblerService *ServiceByName(const std::string &name) const;
  std::vector<ScrobblerService *> All() const;
  Signal<std::string> Error;
  Signal<bool> EnabledChanged;
  Signal<Song> TrackLoved;

 private:
  NetworkAccessManager *network_;
  std::vector<std::unique_ptr<ScrobblerService>> services_;
};

#endif
