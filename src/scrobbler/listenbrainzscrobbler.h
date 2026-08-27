#ifndef STRAWBERRY_LISTENBRAINZSCROBBLER_H
#define STRAWBERRY_LISTENBRAINZSCROBBLER_H

#include "scrobbler/audioscrobbler.h"
#include "scrobbler/scrobblercache.h"

#include <cstdint>
#include <string>
#include <vector>

class ListenBrainzScrobbler : public ScrobblerService {
 public:
  static const char *kSubmitUrl;
  static const char *kFeedbackUrl;
  static const char *kCacheFile;

  explicit ListenBrainzScrobbler(NetworkAccessManager *network);

  std::string name() const override { return "ListenBrainz"; }
  void NowPlaying(const Song &song) override;
  void ClearPlaying() override;
  void Scrobble(const Song &song) override;
  void Love(const Song &song) override;
  void Authenticate(const std::string &username, const std::string &token) override;
  void Logout() override;
  bool authenticated() const override { return !token_.empty(); }
  std::string username() const override { return username_; }
  void WriteCache() override;
  void Submit() override;

  static std::string SubmitBody(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items);
  static std::string LoveBody(const std::string &recording_mbid);

 private:
  void Submit(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items, bool from_cache);
  void CheckScrobblePrevSong();

  NetworkAccessManager *network_ = nullptr;
  std::string token_;
  std::string username_;
  ScrobblerCache cache_;
  Song song_playing_;
  uint64_t timestamp_ = 0;
  bool scrobbled_ = false;
};

#endif
