#include "tagfetcher/musicbrainzclient.h"

#include "core/networktimeoutpolicy.h"
#include "tagfetcher/musicbrainzdiscid.h"
#include "utilities/jsonutils.h"

#include <glib.h>

MusicBrainzClient::MusicBrainzClient(NetworkAccessManager *network) : network_(network) {
  timeouts_.SetTimeout(NetworkTimeoutPolicy::kMusicBrainzTimeoutMs);
  timeouts_.SetAbort([this](int req_id) {
    if (network_) {
      network_->Cancel(req_id);
    }
  });
}

MusicBrainzClient::~MusicBrainzClient() {
  *alive_ = false;
  CancelAll();
}

MusicBrainzClient::ResultList MusicBrainzClient::ParseDiscResults(const std::string &json, const std::string &disc_id) {
  return MusicBrainzDiscId::ParseDiscResults(json, disc_id);
}

MusicBrainzClient::ResultList MusicBrainzClient::ParseResults(const std::string &json) {
  ResultList results;
  for (const Song &song : JsonUtils::ParseMusicBrainzRecordings(json)) {
    Result result;
    result.title = song.title();
    result.artist = song.artist();
    result.album = song.album();
    result.album_artist = song.albumartist();
    result.track = song.track();
    result.year = song.year();
    result.duration_msec = song.length_nanosec() > 0 ? static_cast<int>(song.length_nanosec() / 1000000) : 0;
    result.musicbrainz_recording_id = song.musicbrainz_recording_id();
    result.musicbrainz_artist_id = song.musicbrainz_artist_id();
    result.musicbrainz_album_id = song.musicbrainz_album_id();
    result.musicbrainz_album_artist_id = song.musicbrainz_album_artist_id();
    results.push_back(result);
  }
  return results;
}

SongList MusicBrainzClient::ToSongs(const ResultList &results) {
  SongList songs;
  for (const Result &result : results) {
    Song song;
    song.set_title(result.title);
    song.set_artist(result.artist);
    song.set_album(result.album);
    song.set_albumartist(result.album_artist);
    song.set_track(result.track);
    song.set_year(result.year);
    if (result.duration_msec > 0) {
      song.set_length_nanosec(static_cast<int64_t>(result.duration_msec) * 1000000);
    }
    song.set_musicbrainz_recording_id(result.musicbrainz_recording_id);
    song.set_musicbrainz_artist_id(result.musicbrainz_artist_id);
    song.set_musicbrainz_album_id(result.musicbrainz_album_id);
    song.set_musicbrainz_album_artist_id(result.musicbrainz_album_artist_id);
    song.set_valid(true);
    songs.push_back(song);
  }
  return songs;
}

void MusicBrainzClient::StartDiscId(const std::string &disc_id) {
  if (!network_ || disc_id.empty()) {
    DiscIdFinished.Emit(disc_id, {}, "No MusicBrainz disc ID");
    return;
  }
  const std::string url = MusicBrainzDiscId::DiscUrl(disc_id);
  const int req = network_->Get(
      url,
      [this, disc_id, alive = alive_](const NetworkAccessManager::Response &response) {
        if (!*alive) {
          return;
        }
        if (!response.ok()) {
          DiscIdFinished.Emit(disc_id, {}, NetworkTimeoutPolicy::FailureMessage(response.error, "MusicBrainz request failed"));
          return;
        }
        DiscIdFinished.Emit(disc_id, ParseDiscResults(response.body, disc_id), {});
      },
      {{"User-Agent", "Strawberry/1.0 (https://www.strawberrymusicplayer.org/)"}});
  timeouts_.AddReply(req);
}

void MusicBrainzClient::Start(int id, const std::vector<std::string> &mbid_list) {
  if (!network_ || mbid_list.empty()) {
    Finished.Emit(id, {}, "No MusicBrainz IDs");
    return;
  }
  const std::string url = std::string("https://musicbrainz.org/ws/2/recording/") + mbid_list.front() +
                          "?inc=artists+releases+release-groups&fmt=json";
  const int req = network_->Get(
      url,
      [this, id, alive = alive_](const NetworkAccessManager::Response &response) {
        if (!*alive) {
          return;
        }
        auto it = requests_.find(id);
        if (it != requests_.end()) {
          timeouts_.Cancel(it->second);
          requests_.erase(it);
        }
        if (!response.ok()) {
          Finished.Emit(id, {}, NetworkTimeoutPolicy::FailureMessage(response.error, "MusicBrainz request failed"));
          return;
        }
        Finished.Emit(id, ParseResults(response.body), {});
      },
      {{"User-Agent", "Strawberry/1.0 (https://www.strawberrymusicplayer.org/)"}});
  requests_[id] = req;
  timeouts_.AddReply(req);
}

void MusicBrainzClient::Cancel(int id) {
  auto it = requests_.find(id);
  if (it == requests_.end()) {
    return;
  }
  timeouts_.Cancel(it->second);
  if (network_) {
    network_->Cancel(it->second);
  }
  requests_.erase(it);
}

void MusicBrainzClient::CancelAll() {
  for (const auto &entry : requests_) {
    timeouts_.Cancel(entry.second);
    if (network_) {
      network_->Cancel(entry.second);
    }
  }
  requests_.clear();
}
