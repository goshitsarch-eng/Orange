#ifndef STRAWBERRY_ALBUMCOVERMANAGERLIST_H
#define STRAWBERRY_ALBUMCOVERMANAGERLIST_H

#include "core/song.h"

#include <string>
#include <vector>

class AlbumCoverManagerList {
 public:
  void SetSongs(const SongList &songs);
  const std::vector<Song> &albums() const { return albums_; }
  int album_count() const { return static_cast<int>(albums_.size()); }
  int with_cover_count() const { return with_cover_; }

 private:
  std::vector<Song> albums_;
  int with_cover_ = 0;
};

#endif
