#ifndef STRAWBERRY_SCROBBLEMETADATA_H
#define STRAWBERRY_SCROBBLEMETADATA_H

#include "core/song.h"

#include <cstdint>
#include <string>

struct ScrobbleMetadata {
  std::string artist;
  std::string album;
  std::string title;
  std::string albumartist;
  int track = 0;
  int64_t length_nanosec = 0;
  uint64_t timestamp = 0;
  std::string musicbrainz_album_artist_id;
  std::string musicbrainz_artist_id;
  std::string musicbrainz_original_artist_id;
  std::string musicbrainz_album_id;
  std::string musicbrainz_original_album_id;
  std::string musicbrainz_recording_id;
  std::string musicbrainz_track_id;
  std::string musicbrainz_disc_id;
  std::string musicbrainz_release_group_id;
  std::string musicbrainz_work_id;
  std::string music_service;
  std::string music_service_name;
  std::string share_url;
  std::string spotify_id;

  static std::string StripRemasteredTitle(const std::string &title);
  static ScrobbleMetadata FromSong(const Song &song, uint64_t timestamp = 0, bool prefer_album_artist = false,
                                  bool strip_remastered = false);
  static ScrobbleMetadata FromSongSettings(const Song &song, uint64_t timestamp = 0);
};

#endif
