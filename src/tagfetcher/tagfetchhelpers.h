#ifndef STRAWBERRY_TAGFETCHHELPERS_H
#define STRAWBERRY_TAGFETCHHELPERS_H

#include "core/song.h"
#include "tagfetcher/musicbrainzclient.h"

#include <string>

namespace TagFetchHelpers {

inline MusicBrainzClient::Result ResultFromSong(const Song &song) {
  MusicBrainzClient::Result result;
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
  return result;
}

inline Song ApplyTags(const Song &original, const Song &result) {
  Song song = original;
  if (!result.title().empty()) {
    song.set_title(result.title());
  }
  if (!result.artist().empty()) {
    song.set_artist(result.artist());
  }
  if (!result.album().empty()) {
    song.set_album(result.album());
  }
  if (!result.albumartist().empty()) {
    song.set_albumartist(result.albumartist());
  }
  if (result.track() > 0) {
    song.set_track(result.track());
  }
  if (result.year() > 0) {
    song.set_year(result.year());
  }
  if (result.length_nanosec() > 0) {
    song.set_length_nanosec(result.length_nanosec());
  }
  if (!result.musicbrainz_recording_id().empty()) {
    song.set_musicbrainz_recording_id(result.musicbrainz_recording_id());
  }
  if (!result.musicbrainz_artist_id().empty()) {
    song.set_musicbrainz_artist_id(result.musicbrainz_artist_id());
  }
  if (!result.musicbrainz_album_id().empty()) {
    song.set_musicbrainz_album_id(result.musicbrainz_album_id());
  }
  if (!result.musicbrainz_album_artist_id().empty()) {
    song.set_musicbrainz_album_artist_id(result.musicbrainz_album_artist_id());
  }
  return song;
}

inline Song ApplyResult(const Song &original, const MusicBrainzClient::Result &result) {
  return ApplyTags(original, MusicBrainzClient::ToSongs({result}).front());
}

struct BatchProgress {
  int completed = 0;
  int total = 0;
  int current_id = 0;

  static BatchProgress FromCounts(int completed, int total, int current_id = 0) {
    BatchProgress progress;
    progress.completed = completed;
    progress.total = total;
    progress.current_id = current_id;
    return progress;
  }

  double Fraction() const { return total > 0 ? static_cast<double>(completed) / static_cast<double>(total) : 0.0; }
  bool Done() const { return total > 0 && completed >= total; }
  std::string StatusText() const {
    if (total <= 0) {
      return {};
    }
    return std::to_string(completed) + " / " + std::to_string(total);
  }
};

}  // namespace TagFetchHelpers

#endif
