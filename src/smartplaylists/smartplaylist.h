#ifndef STRAWBERRY_SMARTPLAYLIST_H
#define STRAWBERRY_SMARTPLAYLIST_H

#include "core/song.h"

#include <string>
#include <vector>

class CollectionBackend;

enum class SmartPlaylistField {
  Title,
  Album,
  Artist,
  AlbumArtist,
  Composer,
  Genre,
  Year,
  Rating,
  Playcount,
  Skipcount,
  Length,
  Bitrate,
  DateCreated,
  LastPlayed,
  Track,
  Disc,
  OriginalYear,
  Performer,
  Grouping,
  Comment,
  Filepath,
  Filetype,
  Filesize,
  DateModified,
  Samplerate,
  Bitdepth,
  BPM,
  Mood,
  InitialKey
};

enum class SmartPlaylistOp {
  Contains,
  NotContains,
  Equals,
  GreaterThan,
  LessThan,
  StartsWith,
  EndsWith,
  NotEquals,
  Empty,
  NotEmpty
};

struct SmartPlaylistTerm {
  SmartPlaylistField field = SmartPlaylistField::Title;
  SmartPlaylistOp op = SmartPlaylistOp::Contains;
  std::string value;
  bool Matches(const Song &song) const;
};

class SmartPlaylistSearch {
 public:
  enum class SearchType { And, Or };

  SearchType type = SearchType::And;
  std::vector<SmartPlaylistTerm> terms;
  int limit = 0;
  SmartPlaylistField sort_field = SmartPlaylistField::Title;
  bool sort_descending = false;

  SongList Search(const SongList &songs) const;
  SongList Search(CollectionBackend *backend) const;

  static std::vector<std::string> FieldNames();
  static std::vector<std::string> OpNames();
  static SmartPlaylistField FieldFromIndex(int index);
  static SmartPlaylistOp OpFromIndex(int index);
};

#endif  // STRAWBERRY_SMARTPLAYLIST_H
