#include "spotify/spotifyservice.h"

#include "core/settings.h"
#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

const char SpotifyService::kApiUrl[] = "https://api.spotify.com/v1";

SpotifyService::SpotifyService(NetworkAccessManager *network) : network_(network) { ReloadSettings(); }

void SpotifyService::ReloadSettings() {
  Settings settings;
  settings.BeginGroup("Spotify");
  token_ = settings.Value("token");
  if (token_.empty()) {
    token_ = settings.Value("access_token");
  }
  logged_in_ = !token_.empty();
}

void SpotifyService::Login(const std::string &username, const std::string &password_or_token) {
  Settings settings;
  settings.BeginGroup("Spotify");
  if (!username.empty()) {
    settings.SetValue("username", username);
  }
  settings.SetValue("token", password_or_token);
  settings.SetValue("access_token", password_or_token);
  settings.Sync();
  ReloadSettings();
}

std::map<std::string, std::string> SpotifyService::AuthHeaders() const {
  if (token_.empty()) {
    return {};
  }
  return {{"Authorization", "Bearer " + token_}};
}

void SpotifyService::Search(const std::string &query, SearchCallback callback) {
  if (!network_ || query.empty()) {
    callback({});
    return;
  }
  const std::string url = std::string(kApiUrl) + "/search?type=track&limit=50&q=" + StrUtils::UriEscape(query);
  network_->Get(url, [callback](const NetworkAccessManager::Response &response) {
    if (!response.ok()) {
      callback({});
      return;
    }
    callback(JsonUtils::ParseSpotifyTracks(response.body));
  }, AuthHeaders());
}

UrlHandler::LoadResult SpotifyService::Load(const std::string &url, AsyncCallback callback) {
  LoadResult result;
  result.media_url = url;
  std::string id = url;
  const auto scheme = id.find("://");
  if (scheme != std::string::npos) {
    id = id.substr(scheme + 3);
  }
  if (!network_ || id.empty() || token_.empty()) {
    result.error = "Spotify is not signed in";
    if (callback) {
      callback(result);
    }
    return result;
  }
  result.type = LoadResult::Type::Async;
  const std::string request = std::string(kApiUrl) + "/tracks/" + StrUtils::UriEscape(id);
  network_->Get(request, [callback, url](const NetworkAccessManager::Response &response) {
    LoadResult async;
    async.media_url = url;
    if (response.ok()) {
      async.stream_url = JsonUtils::GetString(response.body, {"preview_url"});
      if (async.stream_url.empty()) {
        const SongList songs = JsonUtils::ParseSpotifyTracks(response.body);
        if (!songs.empty()) {
          async.stream_url = songs.front().stream_url();
        }
      }
    }
    async.type = async.stream_url.empty() ? LoadResult::Type::Error : LoadResult::Type::TrackAvailable;
    if (async.stream_url.empty()) {
      async.error = response.error.empty() ? "Spotify preview URL missing" : response.error;
    }
    if (callback) {
      callback(async);
    }
  }, AuthHeaders());
  return result;
}
