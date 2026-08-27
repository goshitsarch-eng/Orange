#include "scrobbler/subsonicscrobbler.h"

#include "constants/subsonicsettings.h"
#include "core/settings.h"
#include "scrobbler/subsonicscrobblestate.h"
#include "subsonic/subsonicservice.h"

#include <ctime>
#include <map>

SubsonicScrobbler::SubsonicScrobbler(NetworkAccessManager *network) : network_(network) {}

std::string SubsonicScrobbler::ScrobbleUrl(const std::string &server_url, const std::string &username, const std::string &password,
                                           const std::string &id, bool submission, bool hex_auth, int64_t time_ms) {
  std::map<std::string, std::string> params = {{"id", id}, {"submission", submission ? "true" : "false"}};
  if (time_ms > 0) {
    params.emplace("time", std::to_string(time_ms));
  }
  return SubsonicService::CreateUrl(server_url, username, password, "scrobble", params, hex_auth);
}

void SubsonicScrobbler::Ping(const Song &song, bool submission, int64_t time_ms) {
  if (!enabled_ || !network_) {
    return;
  }
  Settings settings;
  settings.BeginGroup(SubsonicSettings::kSettingsGroup);
  const std::string url = settings.Value(SubsonicSettings::kUrl);
  const std::string user = settings.Value(SubsonicSettings::kUsername);
  const std::string password = settings.Value(SubsonicSettings::kPassword);
  const bool hex_auth = settings.Contains("hexauth")
                            ? settings.BoolValue("hexauth", false)
                            : settings.IntValue(SubsonicSettings::kAuthMethod, static_cast<int>(SubsonicSettings::kDefaultAuthMethod)) ==
                                  static_cast<int>(SubsonicSettings::AuthMethod::Hex);
  if (url.empty() || user.empty()) {
    return;
  }
  const std::string id = SubsonicScrobbleState::TrackId(song);
  if (id.empty()) {
    return;
  }
  network_->Get(ScrobbleUrl(url, user, password, id, submission, hex_auth, time_ms), [](const NetworkAccessManager::Response &) {});
}

void SubsonicScrobbler::NowPlaying(const Song &song) {
  Settings settings;
  settings.BeginGroup(SubsonicSettings::kSettingsGroup);
  const bool server_side =
      settings.BoolValue(SubsonicSettings::kServerSideScrobbling, SubsonicSettings::kDefaultServerSideScrobbling);
  if (!SubsonicScrobbleState::ShouldNowPlaying(song, server_side)) {
    return;
  }
  playing_url_ = song.url();
  playing_song_id_ = song.song_id();
  playing_time_ms_ = static_cast<int64_t>(std::time(nullptr)) * 1000;
  Ping(song, false, playing_time_ms_);
}

void SubsonicScrobbler::Scrobble(const Song &song) {
  Settings settings;
  settings.BeginGroup(SubsonicSettings::kSettingsGroup);
  const bool server_side =
      settings.BoolValue(SubsonicSettings::kServerSideScrobbling, SubsonicSettings::kDefaultServerSideScrobbling);
  if (!server_side || !SubsonicScrobbleState::ShouldSubmit(song, playing_url_, playing_song_id_)) {
    return;
  }
  Ping(song, true, playing_time_ms_);
}

void SubsonicScrobbler::Love(const Song &) {}

void SubsonicScrobbler::Authenticate(const std::string &username, const std::string &password) {
  Settings settings;
  settings.BeginGroup(SubsonicSettings::kSettingsGroup);
  settings.SetValue(SubsonicSettings::kUsername, username);
  settings.SetValue(SubsonicSettings::kPassword, password);
  settings.Sync();
}
