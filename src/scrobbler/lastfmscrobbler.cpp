#include "scrobbler/lastfmscrobbler.h"

#include "core/settings.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <glib.h>

#include <ctime>

const char *LastFmScrobbler::kApiUrl = "https://ws.audioscrobbler.com/2.0/";
const char *LastFmScrobbler::kApiKey = "211990b4c96782c05d1536e7219eb56e";
const char *LastFmScrobbler::kSecret = "80fd738f49596e9709b1bf9319c444a8";
const char *LastFmScrobbler::kCacheFile = "lastfmscrobbler.cache";

LastFmScrobbler::LastFmScrobbler(NetworkAccessManager *network) : network_(network), cache_(kCacheFile) {
  Settings settings;
  settings.BeginGroup("Last.fm");
  session_key_ = settings.Value("session_key");
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

void LastFmScrobbler::Post(const std::map<std::string, std::string> &params) {
  if (!enabled_ || !network_ || session_key_.empty()) {
    return;
  }
  std::map<std::string, std::string> signed_params = params;
  signed_params["api_sig"] = Sign(signed_params);
  signed_params["format"] = "json";
  network_->Post(kApiUrl, FormBody(signed_params), [](const NetworkAccessManager::Response &) {}, "application/x-www-form-urlencoded");
}

void LastFmScrobbler::NowPlaying(const Song &song) {
  if (session_key_.empty()) {
    return;
  }
  Post({{"method", "track.updateNowPlaying"},
        {"api_key", kApiKey},
        {"sk", session_key_},
        {"artist", song.artist()},
        {"track", song.title()},
        {"album", song.album()}});
}

void LastFmScrobbler::Scrobble(const Song &song) {
  cache_.Add(song, static_cast<uint64_t>(std::time(nullptr)));
  SubmitCache();
}

void LastFmScrobbler::SubmitCache() {
  const std::vector<ScrobblerCacheItem> unsent = cache_.Unsent();
  if (unsent.empty() || session_key_.empty() || !network_) {
    return;
  }
  cache_.MarkSent();
  std::map<std::string, std::string> params = ScrobbleParams(unsent, session_key_);
  params["api_sig"] = Sign(params);
  params["format"] = "json";
  network_->Post(kApiUrl, FormBody(params), [this](const NetworkAccessManager::Response &response) {
    if (response.ok()) {
      cache_.RemoveSent();
    }
  }, "application/x-www-form-urlencoded");
}

void LastFmScrobbler::Love(const Song &song) {
  Post({{"method", "track.love"},
        {"api_key", kApiKey},
        {"sk", session_key_},
        {"artist", song.artist()},
        {"track", song.title()}});
}

void LastFmScrobbler::Authenticate(const std::string &username, const std::string &password) {
  Settings settings;
  settings.BeginGroup("Last.fm");
  settings.SetValue("username", username);
  settings.SetValue("password", password);
  settings.Sync();
  if (!network_) {
    return;
  }
  std::map<std::string, std::string> params = {{"method", "auth.getMobileSession"},
                                               {"username", username},
                                               {"password", password},
                                               {"api_key", kApiKey}};
  params["api_sig"] = Sign(params);
  params["format"] = "json";
  network_->Post(kApiUrl, FormBody(params), [this](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      return;
    }
    const std::string key = JsonUtils::GetString(response.body, {"session", "key"});
    if (!key.empty()) {
      session_key_ = key;
      Settings saved;
      saved.BeginGroup("Last.fm");
      saved.SetValue("session_key", key);
      saved.Sync();
    }
  }, "application/x-www-form-urlencoded");
}
