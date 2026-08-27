#include "scrobbler/subsonicscrobbler.h"

#include "constants/scrobblersettings.h"
#include "constants/subsonicsettings.h"
#include "core/settings.h"
#include "scrobbler/subsonicscrobblestate.h"
#include "subsonic/subsonicservice.h"

#include <glib.h>

#include <ctime>
#include <map>

SubsonicScrobbler::SubsonicScrobbler(NetworkAccessManager *network) : network_(network), pending_song_(Song()) {}

SubsonicScrobbler::~SubsonicScrobbler() { CancelSubmitTimer(); }

void SubsonicScrobbler::CancelSubmitTimer() {
  if (submit_timeout_id_ != 0) {
    g_source_remove(submit_timeout_id_);
    submit_timeout_id_ = 0;
  }
}

void SubsonicScrobbler::SubmitPending() {
  submitted_ = false;
  CancelSubmitTimer();
  if (!pending_song_.is_valid() && pending_song_.song_id().empty() && pending_song_.url().empty()) {
    return;
  }
  Ping(pending_song_, true, playing_time_ms_);
}

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
  CancelSubmitTimer();
  submitted_ = false;
  pending_song_ = Song();
  playing_url_ = song.url();
  playing_song_id_ = song.song_id();
  playing_time_ms_ = static_cast<int64_t>(std::time(nullptr)) * 1000;
  Ping(song, false, playing_time_ms_);
}

void SubsonicScrobbler::ClearPlaying() {
  CancelSubmitTimer();
  submitted_ = false;
  pending_song_ = Song();
  playing_url_.clear();
  playing_song_id_.clear();
  playing_time_ms_ = 0;
}

void SubsonicScrobbler::Scrobble(const Song &song) {
  Settings settings;
  settings.BeginGroup(SubsonicSettings::kSettingsGroup);
  const bool server_side =
      settings.BoolValue(SubsonicSettings::kServerSideScrobbling, SubsonicSettings::kDefaultServerSideScrobbling);
  if (!server_side || !SubsonicScrobbleState::ShouldSubmit(song, playing_url_, playing_song_id_)) {
    return;
  }
  if (submitted_) {
    return;
  }
  Settings scrobbler;
  scrobbler.BeginGroup(ScrobblerSettings::kSettingsGroup);
  const int delay = scrobbler.IntValue(ScrobblerSettings::kSubmit, ScrobblerSettings::kDefaultSubmit);
  pending_song_ = song;
  if (SubsonicScrobbleState::ShouldSubmitImmediately(delay, submitted_)) {
    submitted_ = true;
    SubmitPending();
    return;
  }
  if (SubsonicScrobbleState::ShouldStartSubmitTimer(delay, submitted_, submit_timeout_id_ != 0)) {
    submitted_ = true;
    submit_timeout_id_ = g_timeout_add_seconds(static_cast<guint>(SubsonicScrobbleState::DelaySeconds(delay)), +[](gpointer data) -> gboolean {
                                                 auto *self = static_cast<SubsonicScrobbler *>(data);
                                                 self->submit_timeout_id_ = 0;
                                                 self->SubmitPending();
                                                 return G_SOURCE_REMOVE;
                                               },
                                               this);
  }
}

void SubsonicScrobbler::Love(const Song &) {}

void SubsonicScrobbler::Authenticate(const std::string &username, const std::string &password) {
  Settings settings;
  settings.BeginGroup(SubsonicSettings::kSettingsGroup);
  settings.SetValue(SubsonicSettings::kUsername, username);
  settings.SetValue(SubsonicSettings::kPassword, password);
  settings.Sync();
}
