#ifndef STRAWBERRY_STREAMSERVICEPLAYLISTITEM_H
#define STRAWBERRY_STREAMSERVICEPLAYLISTITEM_H

#include "core/song.h"

#include <string>

class StreamServicePlaylistItem {
 public:
  StreamServicePlaylistItem() = default;
  explicit StreamServicePlaylistItem(const Song &song);

  const Song &song() const { return song_; }
  Song::Source source() const { return song_.source(); }
  const std::string &url() const { return song_.url(); }
  std::string DisplayText() const;

 private:
  Song song_;
};

#endif
