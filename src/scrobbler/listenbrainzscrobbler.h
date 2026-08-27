#ifndef STRAWBERRY_LISTENBRAINZSCROBBLER_H
#define STRAWBERRY_LISTENBRAINZSCROBBLER_H

#include "scrobbler/audioscrobbler.h"
#include "scrobbler/scrobblercache.h"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

class LocalRedirectServer;

class ListenBrainzScrobbler : public ScrobblerService {
 public:
  static const char *kSubmitUrl;
  static const char *kFeedbackUrl;
  static const char *kCacheFile;

  explicit ListenBrainzScrobbler(NetworkAccessManager *network);
  ~ListenBrainzScrobbler() override;

  std::string name() const override { return "ListenBrainz"; }
  void NowPlaying(const Song &song) override;
  void ClearPlaying() override;
  void Scrobble(const Song &song) override;
  void Love(const Song &song) override;
  void Authenticate(const std::string &username, const std::string &token) override;
  void StartAuthorization(NetworkAccessManager *network, std::function<void(bool)> done = {});
  void Logout() override;
  bool authenticated() const override { return !token_.empty(); }
  bool oauth_authenticated() const { return !access_token_.empty(); }
  std::string username() const override { return username_; }
  void WriteCache() override;
  void Submit() override;

  static std::string SubmitBody(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items);
  static std::string LoveBody(const std::string &recording_mbid);

 private:
  void Submit(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items, bool from_cache);
  void CheckScrobblePrevSong();
  void CancelSubmitTimer();
  void ScheduleSubmit(bool had_error);
  void FlushCache();
  void CloseRedirectServer();
  void CancelOAuthIdle();
  void ExchangeAuthorizationCode(const std::string &code, const std::string &redirect_uri, const std::string &verifier,
                                 std::function<void(bool)> done);
  void SaveAccessToken() const;

  NetworkAccessManager *network_ = nullptr;
  std::string token_;
  std::string username_;
  std::string access_token_;
  std::unique_ptr<LocalRedirectServer> redirect_server_;
  unsigned oauth_idle_id_ = 0;
  ScrobblerCache cache_;
  Song song_playing_;
  uint64_t timestamp_ = 0;
  bool scrobbled_ = false;
  bool submitted_ = false;
  bool submit_error_ = false;
  unsigned submit_timeout_id_ = 0;
};

#endif
