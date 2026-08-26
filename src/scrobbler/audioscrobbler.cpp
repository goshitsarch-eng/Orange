#include "scrobbler/audioscrobbler.h"

#include "config.h"
#include "core/logging.h"
#include "core/settings.h"
#include "utilities/jsonutils.h"

#include <glib.h>

#include <algorithm>
#include <map>
#include <vector>

namespace {

const char *kLastFmUrl = "https://ws.audioscrobbler.com/2.0/";
const char *kLastFmKey = "211990b4c96782c05d1536e7219eb56e";
const char *kLastFmSecret = "80fd738f49596e9709b1bf9319c444a8";

std::string Md5(const std::string &value) {
  gchar *digest = g_compute_checksum_for_string(G_CHECKSUM_MD5, value.c_str(), static_cast<gssize>(value.size()));
  std::string result = digest ? digest : "";
  g_free(digest);
  return result;
}

std::string Sign(const std::map<std::string, std::string> &params) {
  std::string data;
  for (const auto &param : params) {
    data += param.first + param.second;
  }
  data += kLastFmSecret;
  return Md5(data);
}

std::string FormBody(const std::map<std::string, std::string> &params) {
  std::string body;
  for (const auto &param : params) {
    if (!body.empty()) {
      body += "&";
    }
    gchar *key = g_uri_escape_string(param.first.c_str(), nullptr, TRUE);
    gchar *value = g_uri_escape_string(param.second.c_str(), nullptr, TRUE);
    body += std::string(key ? key : "") + "=" + (value ? value : "");
    g_free(key);
    g_free(value);
  }
  return body;
}

class LastFmScrobbler : public ScrobblerService {
 public:
  explicit LastFmScrobbler(NetworkAccessManager *network) : network_(network) {
    Settings s;
    s.BeginGroup("Last.fm");
    session_key_ = s.Value("session_key");
  }
  std::string name() const override { return "Last.fm"; }
  void NowPlaying(const Song &song) override { Post("track.updateNowPlaying", song); }
  void Scrobble(const Song &song) override { Post("track.scrobble", song); }
  void Love(const Song &song) override { Post("track.love", song); }
  void Authenticate(const std::string &username, const std::string &password) override {
    Settings s;
    s.BeginGroup("Last.fm");
    s.SetValue("username", username);
    s.SetValue("password", password);
    s.Sync();
    if (!network_) {
      return;
    }
    std::map<std::string, std::string> params = {{"method", "auth.getMobileSession"},
                                                 {"username", username},
                                                 {"password", password},
                                                 {"api_key", kLastFmKey}};
    params["api_sig"] = Sign(params);
    params["format"] = "json";
    network_->Post(kLastFmUrl, FormBody(params), [this](const NetworkAccessManager::Response &response) {
      if (!response.ok()) {
        return;
      }
      const std::string key = JsonUtils::GetString(response.body, {"session", "key"});
      if (!key.empty()) {
        session_key_ = key;
        Settings settings;
        settings.BeginGroup("Last.fm");
        settings.SetValue("session_key", key);
        settings.Sync();
      }
    }, "application/x-www-form-urlencoded");
  }

 private:
  void Post(const std::string &method, const Song &song) {
    if (!enabled_ || !network_ || session_key_.empty()) {
      return;
    }
    std::map<std::string, std::string> params = {{"method", method},
                                                 {"api_key", kLastFmKey},
                                                 {"sk", session_key_},
                                                 {"artist", song.artist()},
                                                 {"track", song.title()},
                                                 {"album", song.album()}};
    params["api_sig"] = Sign(params);
    params["format"] = "json";
    network_->Post(kLastFmUrl, FormBody(params), [](const NetworkAccessManager::Response &) {}, "application/x-www-form-urlencoded");
  }

  NetworkAccessManager *network_;
  std::string session_key_;
};

class ListenBrainzScrobbler : public ScrobblerService {
 public:
  explicit ListenBrainzScrobbler(NetworkAccessManager *network) : network_(network) {}
  std::string name() const override { return "ListenBrainz"; }
  void NowPlaying(const Song &song) override { Submit("playing_now", song); }
  void Scrobble(const Song &song) override { Submit("single", song); }
  void Love(const Song &) override {}
  void Authenticate(const std::string &username, const std::string &token) override {
    Settings s;
    s.BeginGroup("ListenBrainz");
    s.SetValue("username", username);
    s.SetValue("token", token);
    s.Sync();
    token_ = token;
  }

 private:
  void Submit(const std::string &listen_type, const Song &song) {
    if (!enabled_ || !network_) {
      return;
    }
    Settings s;
    s.BeginGroup("ListenBrainz");
    const std::string token = token_.empty() ? s.Value("token") : token_;
    if (token.empty()) {
      return;
    }
    const std::string body = std::string("{\"listen_type\":\"") + listen_type +
                             "\",\"payload\":[{\"track_metadata\":{\"artist_name\":\"" + song.artist() + "\",\"track_name\":\"" + song.title() +
                             "\",\"release_name\":\"" + song.album() + "\"}}]}";
    network_->Post("https://api.listenbrainz.org/1/submit-listens", body, [](const NetworkAccessManager::Response &) {}, "application/json",
                   {{"Authorization", "Token " + token}});
  }

  NetworkAccessManager *network_;
  std::string token_;
};

#ifdef HAVE_SUBSONIC
class SubsonicScrobbler : public ScrobblerService {
 public:
  explicit SubsonicScrobbler(NetworkAccessManager *network) : network_(network) {}
  std::string name() const override { return "Subsonic"; }
  void NowPlaying(const Song &song) override { Ping(song, false); }
  void Scrobble(const Song &song) override { Ping(song, true); }
  void Love(const Song &) override {}
  void Authenticate(const std::string &username, const std::string &password) override {
    Settings s;
    s.BeginGroup("Subsonic");
    s.SetValue("username", username);
    s.SetValue("password", password);
    s.Sync();
  }

 private:
  void Ping(const Song &song, bool submission) {
    if (!enabled_ || !network_) {
      return;
    }
    Settings s;
    s.BeginGroup("Subsonic");
    const std::string url = s.Value("url");
    const std::string user = s.Value("username");
    const std::string password = s.Value("password");
    if (url.empty() || user.empty()) {
      return;
    }
    gchar *escaped_id = g_uri_escape_string(song.song_id().empty() ? song.url().c_str() : song.song_id().c_str(), nullptr, TRUE);
    const std::string request = url + "/rest/scrobble.view?u=" + user + "&p=" + password + "&v=1.16.1&c=strawberry&id=" +
                                (escaped_id ? escaped_id : "") + (submission ? "&submission=true" : "&submission=false");
    g_free(escaped_id);
    network_->Get(request, [](const NetworkAccessManager::Response &) {});
  }

  NetworkAccessManager *network_;
};
#endif

}  // namespace

AudioScrobbler::AudioScrobbler(NetworkAccessManager *network) : network_(network) {
  services_.push_back(std::make_unique<LastFmScrobbler>(network));
  services_.push_back(std::make_unique<ListenBrainzScrobbler>(network));
#ifdef HAVE_SUBSONIC
  services_.push_back(std::make_unique<SubsonicScrobbler>(network));
#endif
  ReloadSettings();
}

void AudioScrobbler::ReloadSettings() {
  Settings s;
  s.BeginGroup("Scrobbler");
  for (auto &svc : services_) {
    svc->set_enabled(s.BoolValue(svc->name(), false));
  }
}

void AudioScrobbler::NowPlaying(const Song &song) {
  for (auto &s : services_) {
    if (s->enabled()) {
      s->NowPlaying(song);
    }
  }
}

void AudioScrobbler::Scrobble(const Song &song) {
  for (auto &s : services_) {
    if (s->enabled()) {
      s->Scrobble(song);
    }
  }
}

void AudioScrobbler::Love(const Song &song) {
  for (auto &s : services_) {
    if (s->enabled()) {
      s->Love(song);
    }
  }
}

std::vector<ScrobblerService *> AudioScrobbler::All() const {
  std::vector<ScrobblerService *> r;
  for (const auto &s : services_) {
    r.push_back(s.get());
  }
  return r;
}
