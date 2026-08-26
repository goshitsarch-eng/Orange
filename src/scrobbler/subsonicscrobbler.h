#ifndef STRAWBERRY_SUBSONICSCROBBLER_H
#define STRAWBERRY_SUBSONICSCROBBLER_H

#include "scrobbler/audioscrobbler.h"

#include <string>

class SubsonicScrobbler : public ScrobblerService {
 public:
  explicit SubsonicScrobbler(NetworkAccessManager *network);

  std::string name() const override { return "Subsonic"; }
  void NowPlaying(const Song &song) override;
  void Scrobble(const Song &song) override;
  void Love(const Song &song) override;
  void Authenticate(const std::string &username, const std::string &password) override;

  static std::string ScrobbleUrl(const std::string &server_url, const std::string &username, const std::string &password, const std::string &id,
                                 bool submission, bool hex_auth);

 private:
  void Ping(const Song &song, bool submission);

  NetworkAccessManager *network_ = nullptr;
};

#endif
