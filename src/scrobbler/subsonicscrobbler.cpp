#include "scrobbler/subsonicscrobbler.h"

#include "core/settings.h"
#include "subsonic/subsonicservice.h"

SubsonicScrobbler::SubsonicScrobbler(NetworkAccessManager *network) : network_(network) {}

std::string SubsonicScrobbler::ScrobbleUrl(const std::string &server_url, const std::string &username, const std::string &password,
                                           const std::string &id, bool submission, bool hex_auth) {
  return SubsonicService::CreateUrl(server_url, username, password, "scrobble",
                                    {{"id", id}, {"submission", submission ? "true" : "false"}}, hex_auth);
}

void SubsonicScrobbler::Ping(const Song &song, bool submission) {
  if (!enabled_ || !network_) {
    return;
  }
  Settings settings;
  settings.BeginGroup("Subsonic");
  const std::string url = settings.Value("url");
  const std::string user = settings.Value("username");
  const std::string password = settings.Value("password");
  const bool hex_auth = settings.BoolValue("hexauth", false);
  if (url.empty() || user.empty()) {
    return;
  }
  const std::string id = song.song_id().empty() ? song.url() : song.song_id();
  if (id.empty()) {
    return;
  }
  network_->Get(ScrobbleUrl(url, user, password, id, submission, hex_auth), [](const NetworkAccessManager::Response &) {});
}

void SubsonicScrobbler::NowPlaying(const Song &song) { Ping(song, false); }

void SubsonicScrobbler::Scrobble(const Song &song) { Ping(song, true); }

void SubsonicScrobbler::Love(const Song &) {}

void SubsonicScrobbler::Authenticate(const std::string &username, const std::string &password) {
  Settings settings;
  settings.BeginGroup("Subsonic");
  settings.SetValue("username", username);
  settings.SetValue("password", password);
  settings.Sync();
}
