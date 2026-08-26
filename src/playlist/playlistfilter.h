#ifndef STRAWBERRY_PLAYLISTFILTER_H
#define STRAWBERRY_PLAYLISTFILTER_H

#include "core/song.h"

#include <string>
#include <vector>

class PlaylistFilter {
 public:
  void SetFilterString(const std::string &filter_string);
  const std::string &filter_string() const { return filter_string_; }

  bool Accepts(const Song &song) const;
  bool filterAcceptsRow(int source_row, const SongList &songs) const;
  SongList FilterSongs(const SongList &songs) const;
  std::vector<int> VisibleRows(const SongList &songs) const;

 private:
  std::string filter_string_;
};

#endif
