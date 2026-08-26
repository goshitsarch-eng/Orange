#ifndef STRAWBERRY_PLAYLISTTAGCOMPLETION_H
#define STRAWBERRY_PLAYLISTTAGCOMPLETION_H

#include "playlist/playlistdelegates.h"
#include "utilities/strutils.h"

#include <set>
#include <string>
#include <vector>

namespace PlaylistTagCompletion {

inline bool CompletesColumn(PlaylistColumn column) {
  switch (column) {
    case PlaylistColumn::TitleSort:
    case PlaylistColumn::Album:
    case PlaylistColumn::AlbumSort:
    case PlaylistColumn::Artist:
    case PlaylistColumn::ArtistSort:
    case PlaylistColumn::AlbumArtist:
    case PlaylistColumn::AlbumArtistSort:
    case PlaylistColumn::Genre:
    case PlaylistColumn::Composer:
    case PlaylistColumn::ComposerSort:
    case PlaylistColumn::Performer:
    case PlaylistColumn::PerformerSort:
    case PlaylistColumn::Grouping:
      return true;
    default:
      return false;
  }
}

inline std::vector<std::string> UniqueValues(const SongList &songs, PlaylistColumn column) {
  std::set<std::string> values;
  if (!CompletesColumn(column)) {
    return {};
  }
  for (const Song &song : songs) {
    const std::string text = PlaylistDelegates::ColumnText(song, column);
    if (!text.empty()) {
      values.insert(text);
    }
  }
  return {values.begin(), values.end()};
}

inline int FirstPrefixIndex(const std::vector<std::string> &values, const std::string &needle) {
  if (needle.empty()) {
    return -1;
  }
  const std::string n = StrUtils::ToLower(needle);
  for (size_t i = 0; i < values.size(); ++i) {
    if (StrUtils::StartsWith(StrUtils::ToLower(values[i]), n)) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace PlaylistTagCompletion

#endif
