#ifndef STRAWBERRY_SMARTPLAYLIST_H
#define STRAWBERRY_SMARTPLAYLIST_H

#include "core/song.h"

#include <cstdint>
#include <string>
#include <utility>
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
  NotEmpty,
  NumericDate,
  RelativeDate
};

enum class SmartPlaylistFieldKind { Text, Number, Date, Rating, Time };

struct SmartPlaylistTerm {
  SmartPlaylistField field = SmartPlaylistField::Title;
  SmartPlaylistOp op = SmartPlaylistOp::Contains;
  std::string value;
  bool Matches(const Song &song) const;
};

class SmartPlaylistSearch {
 public:
  enum class SearchType { And = 0, Or, All };
  enum class SortType { Random = 0, FieldAsc, FieldDesc };

  SearchType type = SearchType::And;
  std::vector<SmartPlaylistTerm> terms;
  int limit = 0;
  SmartPlaylistField sort_field = SmartPlaylistField::Title;
  bool sort_descending = false;
  bool sort_random = false;

  static const char *TypeName(SearchType type);
  static SearchType TypeFromName(const std::string &name);
  static bool TermsApply(SearchType type);
  bool IsValid() const { return type == SearchType::All || !terms.empty(); }
  SortType sort_type() const {
    if (sort_random) {
      return SortType::Random;
    }
    return sort_descending ? SortType::FieldDesc : SortType::FieldAsc;
  }
  void set_sort_type(SortType sort) {
    sort_random = sort == SortType::Random;
    sort_descending = sort == SortType::FieldDesc;
  }

  SongList Search(const SongList &songs) const;
  SongList Search(CollectionBackend *backend) const;

  std::string Serialize() const;
  static bool Parse(const std::string &blob, SmartPlaylistSearch *search);

  static std::vector<std::pair<std::string, SmartPlaylistSearch>> LoadSaved();
  static void SaveAll(const std::vector<std::pair<std::string, SmartPlaylistSearch>> &presets);
  static void AddSaved(const std::string &name, const SmartPlaylistSearch &search);
  static void RemoveSaved(const std::string &name);
  static bool FindSaved(const std::string &name, SmartPlaylistSearch *search);
  static void RenameSaved(const std::string &old_name, const std::string &new_name, const SmartPlaylistSearch &search);

  static std::vector<std::string> FieldNames();
  static std::vector<std::string> OpNames();
  static std::string OpName(SmartPlaylistOp op);
  static SmartPlaylistField FieldFromIndex(int index);
  static SmartPlaylistOp OpFromIndex(int index);
  static SmartPlaylistFieldKind KindOf(SmartPlaylistField field);
  static std::vector<SmartPlaylistOp> OperatorsFor(SmartPlaylistField field);
  static int64_t ParseDateValue(const std::string &value);
};

#endif  // STRAWBERRY_SMARTPLAYLIST_H
