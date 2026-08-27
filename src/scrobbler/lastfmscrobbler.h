#ifndef STRAWBERRY_LASTFMSCROBBLER_H
#define STRAWBERRY_LASTFMSCROBBLER_H

#include "scrobbler/audioscrobbler.h"
#include "scrobbler/scrobblercache.h"

#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <vector>

class LastFmScrobbler : public ScrobblerService {
 public:
  static const char *kApiUrl;
  static const char *kAuthUrl;
  static const char *kApiKey;
  static const char *kSecret;
  static const char *kCacheFile;

  explicit LastFmScrobbler(NetworkAccessManager *network);
  ~LastFmScrobbler() override;

  std::string name() const override { return "Last.fm"; }
  void NowPlaying(const Song &song) override;
  void ClearPlaying() override;
  void Scrobble(const Song &song) override;
  void Love(const Song &song) override;
  void Authenticate(const std::string &username, const std::string &password) override;
  void StartAuthentication() override;
  void Logout() override;
  bool authenticated() const override { return !session_key_.empty(); }
  std::string username() const override { return username_; }
  void WriteCache() override;
  void Submit() override;

  void Authenticate(const std::string &username, const std::string &password, const std::function<void(bool)> &done);
  void GetToken(const std::function<void(bool)> &done);
  void OpenAuthorizationUrl() const;
  void CompleteAuthorization(const std::function<void(bool)> &done);

  static std::string AuthorizationUrl(const std::string &token);
  static std::string Sign(const std::map<std::string, std::string> &params);
  static std::string FormBody(const std::map<std::string, std::string> &params);
  static std::map<std::string, std::string> ScrobbleParams(const std::vector<ScrobblerCacheItem> &items, const std::string &session_key);

 private:
  void Post(const std::map<std::string, std::string> &params);
  void SubmitCache();
  void ScheduleSubmit(bool had_error);
  void SaveSession();
  void CheckScrobblePrevSong();

  NetworkAccessManager *network_ = nullptr;
  std::string session_key_;
  std::string username_;
  std::string pending_token_;
  ScrobblerCache cache_;
  unsigned submit_timeout_id_ = 0;
  Song song_playing_;
  uint64_t timestamp_ = 0;
  bool scrobbled_ = false;
};

#endif
