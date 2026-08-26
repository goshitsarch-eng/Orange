#include "qobuz/qobuzmetadatarequest.h"

#include "utilities/jsonutils.h"
#include "utilities/strutils.h"

#include <cstdint>
#include <cstdlib>
#include <ctime>

namespace {

constexpr int64_t kNsecPerSec = 1000000000LL;

int YearFromUnix(int64_t released_at) {
  if (released_at <= 0) {
    return -1;
  }
  const time_t value = static_cast<time_t>(released_at);
  const std::tm *utc = gmtime(&value);
  return utc ? utc->tm_year + 1900 : -1;
}

}  // namespace

namespace QobuzMetadataRequest {

std::string Url(const std::string &api_url, const std::string &track_id, const std::string &app_id, const std::string &user_auth_token) {
  std::string url = api_url + "/track/get?track_id=" + StrUtils::UriEscape(track_id);
  if (!app_id.empty()) {
    url += "&app_id=" + StrUtils::UriEscape(app_id);
  }
  if (!user_auth_token.empty()) {
    url += "&user_auth_token=" + StrUtils::UriEscape(user_auth_token);
  }
  return url;
}

Song ParseTrack(const std::string &json) {
  Song song(Song::Source::Qobuz);
  const std::string id = JsonUtils::GetString(json, {"id"});
  if (id.empty()) {
    return song;
  }
  song.set_valid(true);
  song.set_song_id(id);
  song.set_url("qobuz://" + id);
  song.set_title(JsonUtils::GetString(json, {"title"}));
  song.set_track(JsonUtils::GetInt(json, {"track_number"}, -1));
  song.set_disc(JsonUtils::GetInt(json, {"media_number"}, -1));
  const int duration = JsonUtils::GetInt(json, {"duration"}, -1);
  if (duration > 0) {
    song.set_length_nanosec(static_cast<int64_t>(duration) * kNsecPerSec);
  }
  song.set_comment(JsonUtils::GetString(json, {"copyright"}));
  song.set_composer(JsonUtils::GetString(json, {"composer", "name"}));
  song.set_performer(JsonUtils::GetString(json, {"performer", "name"}));
  song.set_album_id(JsonUtils::GetString(json, {"album", "id"}));
  song.set_album(JsonUtils::GetString(json, {"album", "title"}));
  song.set_artist_id(JsonUtils::GetString(json, {"album", "artist", "id"}));
  song.set_artist(JsonUtils::GetString(json, {"album", "artist", "name"}));
  if (song.artist().empty()) {
    song.set_artist(JsonUtils::GetString(json, {"performer", "name"}));
    song.set_artist_id(JsonUtils::GetString(json, {"performer", "id"}));
  }
  song.set_albumartist(song.artist());
  song.set_art_automatic(JsonUtils::GetString(json, {"album", "image", "large"}));
  song.set_genre(JsonUtils::GetString(json, {"album", "genre", "name"}));
  const int year = YearFromUnix(static_cast<int64_t>(JsonUtils::GetDouble(json, {"album", "released_at"}, 0.0)));
  if (year > 0) {
    song.set_year(year);
  }
  return song;
}

void Get(NetworkAccessManager *network, const std::string &url, const std::map<std::string, std::string> &headers, Callback callback) {
  if (!network || url.empty()) {
    if (callback) {
      callback(Song(), "Qobuz metadata request is incomplete");
    }
    return;
  }
  network->Get(
      url,
      [callback](const NetworkAccessManager::Response &response) {
        if (!callback) {
          return;
        }
        if (!response.ok()) {
          callback(Song(), response.error.empty() ? "Qobuz metadata missing" : response.error);
          return;
        }
        const Song song = ParseTrack(response.body);
        callback(song, song.is_valid() ? std::string() : "Qobuz metadata missing");
      },
      headers);
}

}  // namespace QobuzMetadataRequest
