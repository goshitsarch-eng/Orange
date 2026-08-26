#include "scrobbler/audioscrobbler.h"
#include "config.h"
#include "core/settings.h"
#include "core/logging.h"

namespace {
class HttpScrobbler : public ScrobblerService {
 public:
  HttpScrobbler(std::string name, std::string nowplaying, std::string scrobble, NetworkAccessManager *network)
    : name_(std::move(name)), nowplaying_(std::move(nowplaying)), scrobble_(std::move(scrobble)), network_(network) {}
  std::string name() const override { return name_; }
  void NowPlaying(const Song &song) override { Post(nowplaying_, song); }
  void Scrobble(const Song &song) override { Post(scrobble_, song); }
  void Love(const Song &song) override { Post(scrobble_, song); }
  void Authenticate(const std::string &username, const std::string &password) override {
    Settings s; s.BeginGroup(name_); s.SetValue("username", username); s.SetValue("password", password); s.Sync();
  }
 private:
  void Post(const std::string &url, const Song &song) {
    if (!enabled_ || !network_ || url.empty()) return;
    const std::string body = "artist=" + song.artist() + "&track=" + song.title() + "&album=" + song.album();
    network_->Post(url, body, [](const NetworkAccessManager::Response &) {}, "application/x-www-form-urlencoded");
  }
  std::string name_, nowplaying_, scrobble_;
  NetworkAccessManager *network_;
};
}

AudioScrobbler::AudioScrobbler(NetworkAccessManager *network) : network_(network) {
  services_.push_back(std::make_unique<HttpScrobbler>("Last.fm", "https://ws.audioscrobbler.com/2.0/?method=track.updateNowPlaying", "https://ws.audioscrobbler.com/2.0/?method=track.scrobble", network));
  services_.push_back(std::make_unique<HttpScrobbler>("ListenBrainz", "https://api.listenbrainz.org/1/submit-listens", "https://api.listenbrainz.org/1/submit-listens", network));
#ifdef HAVE_SUBSONIC
  services_.push_back(std::make_unique<HttpScrobbler>("Subsonic", "", "", network));
#endif
  ReloadSettings();
}
void AudioScrobbler::ReloadSettings() {
  Settings s; s.BeginGroup("Scrobbler");
  for (auto &svc : services_) svc->set_enabled(s.BoolValue(svc->name(), false));
}
void AudioScrobbler::NowPlaying(const Song &song) { for (auto &s : services_) if (s->enabled()) s->NowPlaying(song); }
void AudioScrobbler::Scrobble(const Song &song) { for (auto &s : services_) if (s->enabled()) s->Scrobble(song); }
void AudioScrobbler::Love(const Song &song) { for (auto &s : services_) if (s->enabled()) s->Love(song); }
std::vector<ScrobblerService *> AudioScrobbler::All() const {
  std::vector<ScrobblerService *> r; for (const auto &s : services_) r.push_back(s.get()); return r;
}
