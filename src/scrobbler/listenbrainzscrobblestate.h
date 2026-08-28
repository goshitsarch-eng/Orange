#ifndef STRAWBERRY_LISTENBRAINZSCROBBLESTATE_H
#define STRAWBERRY_LISTENBRAINZSCROBBLESTATE_H

#include "constants/timeconstants.h"
#include "scrobbler/scrobblercache.h"
#include "utilities/strutils.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace ListenBrainzScrobbleState {

// Qt ListenBrainzScrobbler::Submit uses listen_type "import" for cached scrobbles.
inline const char *CachedListenType() { return "import"; }

// Qt StartSubmit: std::max(submit_delay, submit_error ? 30 : 5) — always at least 5s.
inline int DelaySeconds(int configured, bool had_error) { return std::max(configured, had_error ? 30 : 5); }

inline bool ShouldStartSubmitTimer(bool submitted, bool has_unsent, bool timer_active) {
  return !submitted && has_unsent && !timer_active;
}

inline bool ShouldIncludeReleaseName(const std::string &album) { return !album.empty(); }

inline int DurationMs(int64_t length_nanosec) {
  return length_nanosec > 0 ? static_cast<int>(length_nanosec / TimeConstants::kNsecPerMsec) : 0;
}

inline void AppendUniqueMbids(std::vector<std::string> *ids, const std::string &value) {
  if (!ids || value.empty()) {
    return;
  }
  size_t start = 0;
  while (start < value.size()) {
    const size_t slash = value.find('/', start);
    const std::string id = value.substr(start, slash == std::string::npos ? std::string::npos : slash - start);
    if (!id.empty() && std::find(ids->begin(), ids->end(), id) == ids->end()) {
      ids->push_back(id);
    }
    if (slash == std::string::npos) {
      break;
    }
    start = slash + 1;
  }
}

inline std::string JsonStringArray(const std::vector<std::string> &values) {
  std::string json = "[";
  for (size_t i = 0; i < values.size(); ++i) {
    if (i) {
      json += ",";
    }
    json += "\"" + StrUtils::JsonEscape(values[i]) + "\"";
  }
  json += "]";
  return json;
}

inline std::string ArtistMbidsJson(const ScrobblerCacheItem &item) {
  std::vector<std::string> ids;
  AppendUniqueMbids(&ids, item.musicbrainz_album_artist_id);
  AppendUniqueMbids(&ids, item.musicbrainz_artist_id);
  AppendUniqueMbids(&ids, item.musicbrainz_original_artist_id);
  return ids.empty() ? std::string() : JsonStringArray(ids);
}

inline std::string ReleaseMbid(const ScrobblerCacheItem &item) {
  return item.musicbrainz_album_id.empty() ? item.musicbrainz_original_album_id : item.musicbrainz_album_id;
}

inline void AppendJsonString(std::string *json, const char *key, const std::string &value) {
  if (!json || value.empty()) {
    return;
  }
  *json += ",\"";
  *json += key;
  *json += "\":\"";
  *json += StrUtils::JsonEscape(value);
  *json += "\"";
}

// Qt JsonTrackMetadata additional_info: duration, track, client, MBIDs, streaming origin.
inline std::string AdditionalInfoJson(const ScrobblerCacheItem &item, const std::string &version = {}) {
  std::string json = "{\"media_player\":\"Orange\",\"submission_client\":\"Orange\"";
  if (!version.empty()) {
    const std::string escaped = StrUtils::JsonEscape(version);
    json += ",\"media_player_version\":\"" + escaped + "\",\"submission_client_version\":\"" + escaped + "\"";
  }
  const int duration_ms = DurationMs(item.length_nanosec);
  if (duration_ms > 0) {
    json += ",\"duration_ms\":" + std::to_string(duration_ms);
  }
  if (item.track > 0) {
    json += ",\"tracknumber\":" + std::to_string(item.track);
  }
  const std::string artist_mbids = ArtistMbidsJson(item);
  if (!artist_mbids.empty()) {
    json += ",\"artist_mbids\":" + artist_mbids;
  }
  AppendJsonString(&json, "release_mbid", ReleaseMbid(item));
  AppendJsonString(&json, "release_group_mbid", item.musicbrainz_release_group_id);
  AppendJsonString(&json, "recording_mbid", item.musicbrainz_recording_id);
  AppendJsonString(&json, "track_mbid", item.musicbrainz_track_id);
  std::vector<std::string> work_mbids;
  AppendUniqueMbids(&work_mbids, item.musicbrainz_work_id);
  if (!work_mbids.empty()) {
    json += ",\"work_mbids\":" + JsonStringArray(work_mbids);
  }
  AppendJsonString(&json, "music_service", item.music_service);
  AppendJsonString(&json, "music_service_name", item.music_service_name);
  AppendJsonString(&json, "origin_url", item.share_url);
  AppendJsonString(&json, "spotify_id", item.spotify_id);
  json += "}";
  return json;
}

inline std::string AdditionalInfoJson(int track, int64_t length_nanosec, const std::string &version = {}) {
  ScrobblerCacheItem item;
  item.track = track;
  item.length_nanosec = length_nanosec;
  return AdditionalInfoJson(item, version);
}

}  // namespace ListenBrainzScrobbleState

#endif
