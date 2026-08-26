#ifndef STRAWBERRY_LISTENBRAINZSCROBBLER_H
#define STRAWBERRY_LISTENBRAINZSCROBBLER_H

#include "scrobbler/audioscrobbler.h"
#include "scrobbler/scrobblercache.h"

#include <string>
#include <vector>

class ListenBrainzScrobbler : public ScrobblerService {
 public:
  static const char *kSubmitUrl;
  static const char *kCacheFile;

  explicit ListenBrainzScrobbler(NetworkAccessManager *network);

  std::string name() const override { return "ListenBrainz"; }
  void NowPlaying(const Song &song) override;
  void Scrobble(const Song &song) override;
  void Love(const Song &song) override;
  void Authenticate(const std::string &username, const std::string &token) override;

  static std::string SubmitBody(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items);

 private:
  void Submit(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items, bool from_cache);

  NetworkAccessManager *network_ = nullptr;
  std::string token_;
  ScrobblerCache cache_;
};

#endif
