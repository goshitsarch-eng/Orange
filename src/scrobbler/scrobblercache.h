#ifndef STRAWBERRY_SCROBBLERCACHE_H
#define STRAWBERRY_SCROBBLERCACHE_H

#include "core/song.h"

#include <cstdint>
#include <string>
#include <vector>

struct ScrobblerCacheItem {
  uint64_t timestamp = 0;
  std::string artist;
  std::string album;
  std::string title;
  std::string albumartist;
  int track = 0;
  int64_t length_nanosec = 0;
  bool sent = false;
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
};

class ScrobblerCache {
 public:
  explicit ScrobblerCache(const std::string &filename);

  void Add(const Song &song, uint64_t timestamp);
  std::vector<ScrobblerCacheItem> List() const { return items_; }
  std::vector<ScrobblerCacheItem> Unsent() const;
  void MarkSent();
  void ClearSent();
  void RemoveSent();
  size_t Count() const { return items_.size(); }
  void Load();
  void Save() const;

  static std::string ToJson(const std::vector<ScrobblerCacheItem> &items);
  static std::vector<ScrobblerCacheItem> Parse(const std::string &json);

 private:
  std::string path_;
  std::vector<ScrobblerCacheItem> items_;
};

#endif
