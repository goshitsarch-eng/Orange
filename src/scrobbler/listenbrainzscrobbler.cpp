#include "scrobbler/listenbrainzscrobbler.h"

#include "constants/scrobblersettings.h"
#include "core/settings.h"
#include "scrobbler/scrobblemetadata.h"
#include "scrobbler/scrobblererror.h"
#include "scrobbler/scrobblerplayingstate.h"
#include "utilities/strutils.h"

#include <ctime>

const char *ListenBrainzScrobbler::kSubmitUrl = "https://api.listenbrainz.org/1/submit-listens";
const char *ListenBrainzScrobbler::kFeedbackUrl = "https://api.listenbrainz.org/1/feedback/recording-feedback";
const char *ListenBrainzScrobbler::kCacheFile = "listenbrainzscrobbler.cache";

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

ListenBrainzScrobbler::ListenBrainzScrobbler(NetworkAccessManager *network) : network_(network), cache_(kCacheFile) {
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  token_ = settings.Value("token");
  username_ = settings.Value("username");
}

std::string ListenBrainzScrobbler::SubmitBody(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items) {
  std::string body = "{\"listen_type\":\"" + StrUtils::JsonEscape(listen_type) + "\",\"payload\":[";
  for (size_t i = 0; i < items.size(); ++i) {
    if (i) {
      body += ",";
    }
    body += "{";
    if (listen_type != "playing_now" && items[i].timestamp > 0) {
      body += "\"listened_at\":" + std::to_string(items[i].timestamp) + ",";
    }
    body += "\"track_metadata\":{\"artist_name\":\"" + StrUtils::JsonEscape(items[i].artist) + "\",\"track_name\":\"" +
            StrUtils::JsonEscape(items[i].title) + "\",\"release_name\":\"" + StrUtils::JsonEscape(items[i].album) + "\"}}";
  }
  body += "]}";
  return body;
}

std::string ListenBrainzScrobbler::LoveBody(const std::string &recording_mbid) {
  return std::string("{\"recording_mbid\":\"") + StrUtils::JsonEscape(recording_mbid) + "\",\"score\":1}";
}

void ListenBrainzScrobbler::Submit(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items, bool from_cache) {
  if (!enabled_ || !network_ || items.empty() || OfflineMode()) {
    return;
  }
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  const std::string token = token_.empty() ? settings.Value("token") : token_;
  if (token.empty()) {
    Error.Emit(ScrobblerError::NotAuthenticated("ListenBrainz"));
    return;
  }
  network_->Post(kSubmitUrl, SubmitBody(listen_type, items), [this, from_cache](const NetworkAccessManager::Response &response) {
    if (from_cache && response.ok()) {
      cache_.RemoveSent();
    } else if (!response.ok()) {
      Error.Emit(ScrobblerError::RequestFailed("ListenBrainz"));
    }
  }, "application/json", {{"Authorization", "Token " + token}});
}

void ListenBrainzScrobbler::CheckScrobblePrevSong() {
  const uint64_t now = static_cast<uint64_t>(std::time(nullptr));
  if (!ScrobblerPlayingState::ShouldScrobbleRadioPrev(song_playing_, scrobbled_,
                                                      ScrobblerPlayingState::ElapsedSeconds(timestamp_, now))) {
    return;
  }
  Song song = song_playing_;
  song.set_length_nanosec(ScrobblerPlayingState::ElapsedSeconds(timestamp_, now) * 1000000000LL);
  Scrobble(song);
}

void ListenBrainzScrobbler::NowPlaying(const Song &song) {
  CheckScrobblePrevSong();
  song_playing_ = song;
  timestamp_ = static_cast<uint64_t>(std::time(nullptr));
  scrobbled_ = false;
  if (!song.is_metadata_good()) {
    return;
  }
  const ScrobbleMetadata metadata = ScrobbleMetadata::FromSongSettings(song);
  ScrobblerCacheItem item;
  item.artist = metadata.artist;
  item.album = metadata.album;
  item.title = metadata.title;
  Submit("playing_now", {item}, false);
}

void ListenBrainzScrobbler::ClearPlaying() {
  CheckScrobblePrevSong();
  song_playing_ = Song();
  scrobbled_ = false;
  timestamp_ = 0;
}

void ListenBrainzScrobbler::Scrobble(const Song &song) {
  if (!ScrobblerPlayingState::SameAsPlaying(song, song_playing_)) {
    return;
  }
  scrobbled_ = true;
  cache_.Add(SongFromMetadata(song, ScrobbleMetadata::FromSongSettings(song)),
             ScrobblerPlayingState::TimestampOrNow(timestamp_, static_cast<uint64_t>(std::time(nullptr))));
  const std::vector<ScrobblerCacheItem> items = cache_.Unsent();
  cache_.MarkSent();
  Submit("single", items, true);
}

void ListenBrainzScrobbler::Love(const Song &song) {
  if (!enabled_ || !network_ || song.musicbrainz_recording_id().empty() || OfflineMode()) {
    return;
  }
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  const std::string token = token_.empty() ? settings.Value("token") : token_;
  if (token.empty()) {
    return;
  }
  network_->Post(kFeedbackUrl, LoveBody(song.musicbrainz_recording_id()), [](const NetworkAccessManager::Response &) {}, "application/json",
                 {{"Authorization", "Token " + token}});
}

void ListenBrainzScrobbler::Authenticate(const std::string &username, const std::string &token) {
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  settings.SetValue("username", username);
  settings.SetValue("token", token);
  settings.Sync();
  username_ = username;
  token_ = token;
}

void ListenBrainzScrobbler::Logout() {
  token_.clear();
  username_.clear();
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  settings.SetValue("username", "");
  settings.SetValue("token", "");
  settings.Sync();
}
