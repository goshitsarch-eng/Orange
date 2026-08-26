#ifndef STRAWBERRY_LASTFMSCROBBLER_H
#define STRAWBERRY_LASTFMSCROBBLER_H

#include "scrobbler/audioscrobbler.h"
#include "scrobbler/scrobblercache.h"

#include <map>
#include <string>
#include <vector>

class LastFmScrobbler : public ScrobblerService {
 public:
  static const char *kApiUrl;
  static const char *kApiKey;
  static const char *kSecret;
  static const char *kCacheFile;

  explicit LastFmScrobbler(NetworkAccessManager *network);

  std::string name() const override { return "Last.fm"; }
  void NowPlaying(const Song &song) override;
  void Scrobble(const Song &song) override;
  void Love(const Song &song) override;
  void Authenticate(const std::string &username, const std::string &password) override;

  static std::string Sign(const std::map<std::string, std::string> &params);
  static std::string FormBody(const std::map<std::string, std::string> &params);
  static std::map<std::string, std::string> ScrobbleParams(const std::vector<ScrobblerCacheItem> &items, const std::string &session_key);

 private:
  void Post(const std::map<std::string, std::string> &params);
  void SubmitCache();

  NetworkAccessManager *network_ = nullptr;
  std::string session_key_;
  ScrobblerCache cache_;
};

#endif
