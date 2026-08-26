#include "scrobbler/listenbrainzscrobbler.h"

#include "core/settings.h"
#include "utilities/strutils.h"

#include <ctime>

const char *ListenBrainzScrobbler::kSubmitUrl = "https://api.listenbrainz.org/1/submit-listens";
const char *ListenBrainzScrobbler::kCacheFile = "listenbrainzscrobbler.cache";

ListenBrainzScrobbler::ListenBrainzScrobbler(NetworkAccessManager *network) : network_(network), cache_(kCacheFile) {
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  token_ = settings.Value("token");
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

void ListenBrainzScrobbler::Submit(const std::string &listen_type, const std::vector<ScrobblerCacheItem> &items, bool from_cache) {
  if (!enabled_ || !network_ || items.empty()) {
    return;
  }
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  const std::string token = token_.empty() ? settings.Value("token") : token_;
  if (token.empty()) {
    return;
  }
  network_->Post(kSubmitUrl, SubmitBody(listen_type, items), [this, from_cache](const NetworkAccessManager::Response &response) {
    if (from_cache && response.ok()) {
      cache_.RemoveSent();
    }
  }, "application/json", {{"Authorization", "Token " + token}});
}

void ListenBrainzScrobbler::NowPlaying(const Song &song) {
  ScrobblerCacheItem item;
  item.artist = song.artist();
  item.album = song.album();
  item.title = song.title();
  Submit("playing_now", {item}, false);
}

void ListenBrainzScrobbler::Scrobble(const Song &song) {
  cache_.Add(song, static_cast<uint64_t>(std::time(nullptr)));
  const std::vector<ScrobblerCacheItem> items = cache_.Unsent();
  cache_.MarkSent();
  Submit("single", items, true);
}

void ListenBrainzScrobbler::Love(const Song &) {}

void ListenBrainzScrobbler::Authenticate(const std::string &username, const std::string &token) {
  Settings settings;
  settings.BeginGroup("ListenBrainz");
  settings.SetValue("username", username);
  settings.SetValue("token", token);
  settings.Sync();
  token_ = token;
}
