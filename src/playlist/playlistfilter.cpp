#include "playlist/playlistfilter.h"

void PlaylistFilter::SetFilterString(const std::string &filter_string) {
  filter_string_ = filter_string;
  if (filter_string_.empty()) {
    parser_.reset();
    return;
  }
  parser_ = std::make_unique<FilterParser>(filter_string_);
}

bool PlaylistFilter::Accepts(const Song &song) const {
  if (!parser_) {
    return true;
  }
  return parser_->Matches(song);
}

bool PlaylistFilter::filterAcceptsRow(int source_row, const SongList &songs) const {
  if (source_row < 0 || source_row >= static_cast<int>(songs.size())) {
    return false;
  }
  return Accepts(songs[static_cast<size_t>(source_row)]);
}

SongList PlaylistFilter::FilterSongs(const SongList &songs) const {
  if (!parser_) {
    return songs;
  }
  SongList visible;
  for (const Song &song : songs) {
    if (Accepts(song)) {
      visible.push_back(song);
    }
  }
  return visible;
}

std::vector<int> PlaylistFilter::VisibleRows(const SongList &songs) const {
  std::vector<int> rows;
  for (int i = 0; i < static_cast<int>(songs.size()); ++i) {
    if (filterAcceptsRow(i, songs)) {
      rows.push_back(i);
    }
  }
  return rows;
}
