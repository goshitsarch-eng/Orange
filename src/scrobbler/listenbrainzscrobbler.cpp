#include "scrobbler/listenbrainzscrobbler.h"

#include "constants/scrobblersettings.h"
#include "core/settings.h"
#include "scrobbler/listenbrainzscrobblestate.h"
#include "scrobbler/scrobblemetadata.h"
#include "scrobbler/scrobblererror.h"
#include "scrobbler/scrobblerplayingstate.h"
#include "utilities/strutils.h"

#include <glib.h>

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

ListenBrainzScrobbler::ListenBrainzScrobbler(NetworkAccessManager *network) : network_(network), cache_(kCacheFile), song_playing_(Song()) {
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  token_ = settings.Value("token");
  username_ = settings.Value("username");
}

ListenBrainzScrobbler::~ListenBrainzScrobbler() { CancelSubmitTimer(); }

void ListenBrainzScrobbler::CancelSubmitTimer() {
  if (submit_timeout_id_ != 0) {
    g_source_remove(submit_timeout_id_);
    submit_timeout_id_ = 0;
  }
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
    submitted_ = false;
    if (from_cache) {
      cache_.ClearSent();
    }
    return;
  }
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  const std::string token = token_.empty() ? settings.Value("token") : token_;
  if (token.empty()) {
    submitted_ = false;
    if (from_cache) {
      cache_.ClearSent();
    }
    Error.Emit(ScrobblerError::NotAuthenticated("ListenBrainz"));
    return;
  }
  network_->Post(kSubmitUrl, SubmitBody(listen_type, items), [this, from_cache](const NetworkAccessManager::Response &response) {
    submitted_ = false;
    if (from_cache && response.ok()) {
      cache_.RemoveSent();
      submit_error_ = false;
    } else if (!response.ok()) {
      Error.Emit(ScrobblerError::RequestFailed("ListenBrainz"));
      if (from_cache) {
        cache_.ClearSent();
        ScheduleSubmit(true);
      }
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
  if (OfflineMode() || token_.empty()) {
    return;
  }
  ScheduleSubmit(false);
}

void ListenBrainzScrobbler::WriteCache() { cache_.Save(); }

void ListenBrainzScrobbler::ScheduleSubmit(bool had_error) {
  submit_error_ = had_error;
  if (!ListenBrainzScrobbleState::ShouldStartSubmitTimer(submitted_, !cache_.Unsent().empty(), submit_timeout_id_ != 0)) {
    return;
  }
  Settings settings;
  settings.BeginGroup(ScrobblerSettings::kSettingsGroup);
  const int delay = ListenBrainzScrobbleState::DelaySeconds(settings.IntValue(ScrobblerSettings::kSubmit, ScrobblerSettings::kDefaultSubmit),
                                                            had_error);
  CancelSubmitTimer();
  submit_timeout_id_ = g_timeout_add_seconds(static_cast<guint>(delay), +[](gpointer data) -> gboolean {
                                               auto *self = static_cast<ListenBrainzScrobbler *>(data);
                                               self->submit_timeout_id_ = 0;
                                               self->FlushCache();
                                               return G_SOURCE_REMOVE;
                                             },
                                             this);
}

void ListenBrainzScrobbler::FlushCache() {
  if (submitted_) {
    return;
  }
  const std::vector<ScrobblerCacheItem> items = cache_.Unsent();
  if (items.empty()) {
    return;
  }
  cache_.MarkSent();
  submitted_ = true;
  Submit(ListenBrainzScrobbleState::CachedListenType(), items, true);
}

void ListenBrainzScrobbler::Submit() {
  CancelSubmitTimer();
  FlushCache();
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
