#include "scrobbler/lastfmscrobbler.h"

#include "constants/scrobblersettings.h"
#include "core/settings.h"
#include "scrobbler/scrobblemetadata.h"
#include "scrobbler/scrobblererror.h"
#include "scrobbler/scrobblerplayingstate.h"
#include "scrobbler/scrobblersubmittiming.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <gio/gio.h>
#include <glib.h>

#include <ctime>

const char *LastFmScrobbler::kApiUrl = "https://ws.audioscrobbler.com/2.0/";
const char *LastFmScrobbler::kAuthUrl = "https://www.last.fm/api/auth/";
const char *LastFmScrobbler::kApiKey = "211990b4c96782c05d1536e7219eb56e";
const char *LastFmScrobbler::kSecret = "80fd738f49596e9709b1bf9319c444a8";
const char *LastFmScrobbler::kCacheFile = "lastfmscrobbler.cache";

namespace {

bool OfflineMode() {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  return settings.BoolValue(ScrobblerSettings::kOffline, ScrobblerSettings::kDefaultOffline);
}

Song SongFromMetadata(const Song &song, const ScrobbleMetadata &metadata) {
  Song processed = song;
  processed.set_artist(metadata.artist);
  processed.set_album(metadata.album);
  processed.set_title(metadata.title);
  processed.set_albumartist(metadata.albumartist);
  return processed;
}

}  // namespace

LastFmScrobbler::LastFmScrobbler(NetworkAccessManager *network) : network_(network), cache_(kCacheFile) {
  Settings settings;
  settings.BeginGroup("Last.fm");
  session_key_ = settings.SecretValue("session_key");
  username_ = settings.Value("username");
}

LastFmScrobbler::~LastFmScrobbler() {
  if (submit_timeout_id_ != 0) {
    g_source_remove(submit_timeout_id_);
    submit_timeout_id_ = 0;
  }
}

std::string LastFmScrobbler::AuthorizationUrl(const std::string &token) {
  std::string url = std::string(kAuthUrl) + "?api_key=" + kApiKey;
  if (!token.empty()) {
    url += "&token=" + token;
  }
  return url;
}

std::string LastFmScrobbler::Sign(const std::map<std::string, std::string> &params) {
  std::string data;
  for (const auto &param : params) {
    data += param.first + param.second;
  }
  data += kSecret;
  gchar *digest = g_compute_checksum_for_string(G_CHECKSUM_MD5, data.c_str(), static_cast<gssize>(data.size()));
  std::string result = digest ? digest : "";
  g_free(digest);
  return result;
}

std::string LastFmScrobbler::FormBody(const std::map<std::string, std::string> &params) {
  std::string body;
  for (const auto &param : params) {
    if (!body.empty()) {
      body += "&";
    }
    body += StrUtils::UriEscape(param.first) + "=" + StrUtils::UriEscape(param.second);
  }
  return body;
}

std::map<std::string, std::string> LastFmScrobbler::ScrobbleParams(const std::vector<ScrobblerCacheItem> &items, const std::string &session_key) {
  std::map<std::string, std::string> params = {{"method", "track.scrobble"}, {"api_key", kApiKey}, {"sk", session_key}};
  for (size_t i = 0; i < items.size(); ++i) {
    const std::string index = std::to_string(i);
    params["artist[" + index + "]"] = items[i].artist;
    params["track[" + index + "]"] = items[i].title;
    params["timestamp[" + index + "]"] = std::to_string(items[i].timestamp);
    if (!items[i].album.empty()) {
      params["album[" + index + "]"] = items[i].album;
    }
    if (items[i].length_nanosec > 0) {
      params["duration[" + index + "]"] = std::to_string(items[i].length_nanosec / 1000000000LL);
    }
  }
  return params;
}

void LastFmScrobbler::SaveSession() {
  Settings settings;
  settings.BeginGroup("Last.fm");
  settings.SetSecretValue("session_key", session_key_);
  settings.SetValue("username", username_);
  settings.Sync();
}

void LastFmScrobbler::Post(const std::map<std::string, std::string> &params) {
  if (!enabled_ || !network_ || session_key_.empty() || OfflineMode()) {
    return;
  }
  std::map<std::string, std::string> signed_params = params;
  signed_params["api_sig"] = Sign(signed_params);
  signed_params["format"] = "json";
  network_->Post(
      kApiUrl, FormBody(signed_params),
      [this](const NetworkAccessManager::Response &response) {
        if (!response.ok()) {
          Error.Emit(ScrobblerError::RequestFailed("Last.fm"));
        }
      },
      "application/x-www-form-urlencoded");
}

void LastFmScrobbler::CheckScrobblePrevSong() {
  const uint64_t now = static_cast<uint64_t>(std::time(nullptr));
  if (!ScrobblerPlayingState::ShouldScrobbleRadioPrev(song_playing_, scrobbled_,
                                                      ScrobblerPlayingState::ElapsedSeconds(timestamp_, now))) {
    return;
  }
  Song song = song_playing_;
  song.set_length_nanosec(ScrobblerPlayingState::ElapsedSeconds(timestamp_, now) * 1000000000LL);
  Scrobble(song);
}

void LastFmScrobbler::NowPlaying(const Song &song) {
  CheckScrobblePrevSong();
  song_playing_ = song;
  timestamp_ = static_cast<uint64_t>(std::time(nullptr));
  scrobbled_ = false;
  if (session_key_.empty() || !song.is_metadata_good()) {
    return;
  }
  const ScrobbleMetadata metadata = ScrobbleMetadata::FromSongSettings(song);
  Post({{"method", "track.updateNowPlaying"},
        {"api_key", kApiKey},
        {"sk", session_key_},
        {"artist", metadata.artist},
        {"track", metadata.title},
        {"album", metadata.album}});
}

void LastFmScrobbler::ClearPlaying() {
  CheckScrobblePrevSong();
  song_playing_ = Song();
  scrobbled_ = false;
  timestamp_ = 0;
}

void LastFmScrobbler::Scrobble(const Song &song) {
  if (!ScrobblerPlayingState::SameAsPlaying(song, song_playing_)) {
    return;
  }
  scrobbled_ = true;
  cache_.Add(SongFromMetadata(song, ScrobbleMetadata::FromSongSettings(song)),
             ScrobblerPlayingState::TimestampOrNow(timestamp_, static_cast<uint64_t>(std::time(nullptr))));
  ScheduleSubmit(false);
}

void LastFmScrobbler::WriteCache() { cache_.Save(); }

void LastFmScrobbler::Submit() { SubmitCache(); }

void LastFmScrobbler::ScheduleSubmit(bool had_error) {
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  const int delay = ScrobblerSubmitTiming::DelaySeconds(settings.IntValue(ScrobblerSettings::kSubmit, ScrobblerSettings::kDefaultSubmit),
                                                        had_error);
  if (submit_timeout_id_ != 0) {
    g_source_remove(submit_timeout_id_);
    submit_timeout_id_ = 0;
  }
  if (delay <= 0) {
    SubmitCache();
    return;
  }
  submit_timeout_id_ = g_timeout_add_seconds(static_cast<guint>(delay), +[](gpointer data) -> gboolean {
                                               auto *self = static_cast<LastFmScrobbler *>(data);
                                               self->submit_timeout_id_ = 0;
                                               self->SubmitCache();
                                               return G_SOURCE_REMOVE;
                                             },
                                             this);
}

void LastFmScrobbler::SubmitCache() {
  const std::vector<ScrobblerCacheItem> unsent = cache_.Unsent();
  if (unsent.empty() || session_key_.empty() || !network_ || OfflineMode()) {
    return;
  }
  cache_.MarkSent();
  std::map<std::string, std::string> params = ScrobbleParams(unsent, session_key_);
  params["api_sig"] = Sign(params);
  params["format"] = "json";
  network_->Post(kApiUrl, FormBody(params), [this](const NetworkAccessManager::Response &response) {
    if (response.ok()) {
      cache_.RemoveSent();
    } else {
      Error.Emit(ScrobblerError::RequestFailed("Last.fm"));
      ScheduleSubmit(true);
    }
  }, "application/x-www-form-urlencoded");
}

void LastFmScrobbler::Love(const Song &song) {
  const ScrobbleMetadata metadata = ScrobbleMetadata::FromSongSettings(song);
  Post({{"method", "track.love"},
        {"api_key", kApiKey},
        {"sk", session_key_},
        {"artist", metadata.artist},
        {"track", metadata.title}});
}

void LastFmScrobbler::Authenticate(const std::string &username, const std::string &password) {
  Authenticate(username, password, {});
}

void LastFmScrobbler::Authenticate(const std::string &username, const std::string &password, const std::function<void(bool)> &done) {
  username_ = username;
  Settings settings;
  settings.BeginGroup("Last.fm");
  settings.SetValue("username", username);
  settings.SetValue("password", password);
  settings.Sync();
  if (!network_) {
    Error.Emit(ScrobblerError::NotAuthenticated("Last.fm"));
    if (done) {
      done(false);
    }
    return;
  }
  std::map<std::string, std::string> params = {{"method", "auth.getMobileSession"},
                                               {"username", username},
                                               {"password", password},
                                               {"api_key", kApiKey}};
  params["api_sig"] = Sign(params);
  params["format"] = "json";
  network_->Post(kApiUrl, FormBody(params), [this, done](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      Error.Emit(ScrobblerError::NotAuthenticated("Last.fm"));
      if (done) {
        done(false);
      }
      return;
    }
    const std::string key = JsonUtils::GetString(response.body, {"session", "key"});
    const std::string name = JsonUtils::GetString(response.body, {"session", "name"});
    if (key.empty()) {
      Error.Emit(ScrobblerError::NotAuthenticated("Last.fm"));
      if (done) {
        done(false);
      }
      return;
    }
    session_key_ = key;
    if (!name.empty()) {
      username_ = name;
    }
    SaveSession();
    if (done) {
      done(true);
    }
  }, "application/x-www-form-urlencoded");
}

void LastFmScrobbler::GetToken(const std::function<void(bool)> &done) {
  if (!network_) {
    if (done) {
      done(false);
    }
    return;
  }
  std::map<std::string, std::string> params = {{"method", "auth.getToken"}, {"api_key", kApiKey}};
  params["api_sig"] = Sign(params);
  params["format"] = "json";
  network_->Post(kApiUrl, FormBody(params), [this, done](const NetworkAccessManager::Response &response) {
    const std::string token = response.ok() ? JsonUtils::GetString(response.body, {"token"}) : std::string();
    pending_token_ = token;
    if (done) {
      done(!token.empty());
    }
  }, "application/x-www-form-urlencoded");
}

void LastFmScrobbler::OpenAuthorizationUrl() const {
  if (pending_token_.empty()) {
    return;
  }
  g_app_info_launch_default_for_uri(AuthorizationUrl(pending_token_).c_str(), nullptr, nullptr);
}

void LastFmScrobbler::StartAuthentication() {
  GetToken([this](bool ok) {
    if (ok) {
      OpenAuthorizationUrl();
    }
  });
}

void LastFmScrobbler::CompleteAuthorization(const std::function<void(bool)> &done) {
  if (!network_ || pending_token_.empty()) {
    if (done) {
      done(false);
    }
    return;
  }
  std::map<std::string, std::string> params = {{"method", "auth.getSession"}, {"api_key", kApiKey}, {"token", pending_token_}};
  params["api_sig"] = Sign(params);
  params["format"] = "json";
  network_->Post(kApiUrl, FormBody(params), [this, done](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      if (done) {
        done(false);
      }
      return;
    }
    const std::string key = JsonUtils::GetString(response.body, {"session", "key"});
    const std::string name = JsonUtils::GetString(response.body, {"session", "name"});
    if (key.empty()) {
      if (done) {
        done(false);
      }
      return;
    }
    session_key_ = key;
    if (!name.empty()) {
      username_ = name;
    }
    pending_token_.clear();
    SaveSession();
    if (done) {
      done(true);
    }
  }, "application/x-www-form-urlencoded");
}

void LastFmScrobbler::Logout() {
  session_key_.clear();
  username_.clear();
  pending_token_.clear();
  Settings settings;
  settings.BeginGroup("Last.fm");
  settings.RemoveSecret("session_key");
  settings.SetValue("username", "");
  settings.SetValue("password", "");
  settings.Sync();
}
