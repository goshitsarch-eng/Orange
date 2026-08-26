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
};

class ScrobblerCache {
 public:
  explicit ScrobblerCache(const std::string &filename);

  void Add(const Song &song, uint64_t timestamp);
  std::vector<ScrobblerCacheItem> List() const { return items_; }
  std::vector<ScrobblerCacheItem> Unsent() const;
  void MarkSent();
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
