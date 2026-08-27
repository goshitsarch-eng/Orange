#ifndef STRAWBERRY_SUBSONICSCROBBLER_H
#define STRAWBERRY_SUBSONICSCROBBLER_H

#include "scrobbler/audioscrobbler.h"

#include <cstdint>
#include <string>

class SubsonicScrobbler : public ScrobblerService {
 public:
  explicit SubsonicScrobbler(NetworkAccessManager *network);

  std::string name() const override { return "Subsonic"; }
  ~SubsonicScrobbler() override;

  void NowPlaying(const Song &song) override;
  void ClearPlaying() override;
  void Scrobble(const Song &song) override;
  void Love(const Song &song) override;
  void Authenticate(const std::string &username, const std::string &password) override;

  static std::string ScrobbleUrl(const std::string &server_url, const std::string &username, const std::string &password, const std::string &id,
                                 bool submission, bool hex_auth, int64_t time_ms = 0);

 private:
  void Ping(const Song &song, bool submission, int64_t time_ms);
  void CancelSubmitTimer();
  void SubmitPending();

  NetworkAccessManager *network_ = nullptr;
  std::string playing_url_;
  std::string playing_song_id_;
  int64_t playing_time_ms_ = 0;
  Song pending_song_;
  bool submitted_ = false;
  unsigned submit_timeout_id_ = 0;
};

#endif
