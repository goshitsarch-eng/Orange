#ifndef STRAWBERRY_PLAYLISTFILTER_H
#define STRAWBERRY_PLAYLISTFILTER_H

#include "core/song.h"
#include "filterparser/filterparser.h"

#include <memory>
#include <string>
#include <vector>

class PlaylistFilter {
 public:
  PlaylistFilter() = default;
  PlaylistFilter(PlaylistFilter &&) noexcept = default;
  PlaylistFilter &operator=(PlaylistFilter &&) noexcept = default;
  PlaylistFilter(const PlaylistFilter &) = delete;
  PlaylistFilter &operator=(const PlaylistFilter &) = delete;

  void SetFilterString(const std::string &filter_string);
  const std::string &filter_string() const { return filter_string_; }
  bool has_parser() const { return parser_ != nullptr; }
  const FilterParser *parser() const { return parser_.get(); }

  bool Accepts(const Song &song) const;
  bool filterAcceptsRow(int source_row, const SongList &songs) const;
  SongList FilterSongs(const SongList &songs) const;
  std::vector<int> VisibleRows(const SongList &songs) const;

 private:
  std::string filter_string_;
  std::unique_ptr<FilterParser> parser_;
};

#endif
