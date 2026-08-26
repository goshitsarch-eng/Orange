#ifndef STRAWBERRY_EDITTAGCOMPLETER_H
#define STRAWBERRY_EDITTAGCOMPLETER_H

#include "core/song.h"
#include "playlist/playlisttagcompletion.h"
#include "utilities/strutils.h"

#include <string>
#include <vector>

namespace EditTagCompleter {

inline PlaylistColumn FieldColumn(const std::string &name) {
  if (name == "Artist") {
    return PlaylistColumn::Artist;
  }
  if (name == "Artist sort") {
    return PlaylistColumn::ArtistSort;
  }
  if (name == "Album") {
    return PlaylistColumn::Album;
  }
  if (name == "Album sort") {
    return PlaylistColumn::AlbumSort;
  }
  if (name == "Album artist") {
    return PlaylistColumn::AlbumArtist;
  }
  if (name == "Album artist sort") {
    return PlaylistColumn::AlbumArtistSort;
  }
  if (name == "Genre") {
    return PlaylistColumn::Genre;
  }
  if (name == "Composer") {
    return PlaylistColumn::Composer;
  }
  if (name == "Composer sort") {
    return PlaylistColumn::ComposerSort;
  }
  if (name == "Performer") {
    return PlaylistColumn::Performer;
  }
  if (name == "Performer sort") {
    return PlaylistColumn::PerformerSort;
  }
  if (name == "Grouping") {
    return PlaylistColumn::Grouping;
  }
  if (name == "Title sort") {
    return PlaylistColumn::TitleSort;
  }
  return PlaylistColumn::Title;
}

inline bool CompletesField(const std::string &name) {
  return name == "Artist" || name == "Artist sort" || name == "Album" || name == "Album sort" || name == "Album artist" ||
         name == "Album artist sort" || name == "Genre" || name == "Composer" || name == "Composer sort" || name == "Performer" ||
         name == "Performer sort" || name == "Grouping" || name == "Title sort";
}

inline std::vector<std::string> ValuesFor(const SongList &songs, const std::string &name) {
  if (!CompletesField(name)) {
    return {};
  }
  return PlaylistTagCompletion::UniqueValues(songs, FieldColumn(name));
}

inline std::vector<std::string> Suggestions(const std::vector<std::string> &values, const std::string &needle, int limit = 12) {
  std::vector<std::string> out;
  if (needle.empty() || limit <= 0) {
    return out;
  }
  const std::string n = StrUtils::ToLower(needle);
  for (const std::string &value : values) {
    if (StrUtils::StartsWith(StrUtils::ToLower(value), n) && value != needle) {
      out.push_back(value);
      if (static_cast<int>(out.size()) >= limit) {
        break;
      }
    }
  }
  return out;
}

inline std::string TagsSummary(int count) {
  if (count <= 1) {
    return {};
  }
  return std::to_string(count) + " songs selected.";
}

}  // namespace EditTagCompleter

#endif
