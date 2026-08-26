#ifndef STRAWBERRY_COLLECTIONGROUPING_H
#define STRAWBERRY_COLLECTIONGROUPING_H

#include "core/song.h"

#include <string>
#include <utility>
#include <vector>

class CollectionGrouping {
 public:
  // Values are saved in settings. Do not renumber.
  enum class GroupBy {
    None = 0,
    AlbumArtist = 1,
    Artist = 2,
    Album = 3,
    AlbumDisc = 4,
    YearAlbum = 5,
    YearAlbumDisc = 6,
    OriginalYearAlbum = 7,
    OriginalYearAlbumDisc = 8,
    Disc = 9,
    Year = 10,
    OriginalYear = 11,
    Genre = 12,
    Composer = 13,
    Performer = 14,
    Grouping = 15,
    FileType = 16,
    Format = 17,
    Samplerate = 18,
    Bitdepth = 19,
    Bitrate = 20,
    GroupByCount = 21
  };

  struct Grouping {
    GroupBy first = GroupBy::AlbumArtist;
    GroupBy second = GroupBy::AlbumDisc;
    GroupBy third = GroupBy::None;
    GroupBy operator[](int i) const;
    bool operator==(const Grouping &other) const;
    bool operator!=(const Grouping &other) const { return !(*this == other); }
  };

  struct Node {
    std::string key;
    std::string display;
    std::string sort;
    std::vector<Node> children;
    SongList songs;
  };

  static const char *kComboLabels[];
  static const GroupBy kComboValues[];
  static int ComboCount();
  static int ComboIndex(GroupBy group_by);
  static GroupBy FromComboIndex(int index);
  static std::string Label(GroupBy group_by);
  static GroupBy FromInt(int value);
  static Grouping FromLegacy(const std::string &legacy);

  static std::string TextOrUnknown(const std::string &text);
  static std::string DisplayText(GroupBy group_by, const Song &song);
  static std::string SortText(GroupBy group_by, const Song &song, bool skip_artist_articles, bool skip_album_articles);
  static std::string ContainerKey(GroupBy group_by, const Song &song, bool *has_unique_album_identifier, bool separate_albums_by_grouping);
  static bool IsAlbumGroupBy(GroupBy group_by);
  static bool IsArtistGroupBy(GroupBy group_by);
  static int EffectiveOriginalYear(const Song &song);
  static bool AlbumContainsDisc(const std::string &album);

  static Node BuildTree(const SongList &songs, const Grouping &grouping, bool separate_albums_by_grouping, bool skip_artist_articles,
                        bool skip_album_articles);

  static Grouping LoadCurrent();
  static void SaveCurrent(const Grouping &grouping);
  static bool SeparateAlbumsByGrouping();
  static void SetSeparateAlbumsByGrouping(bool value);

  static std::vector<std::pair<std::string, Grouping>> LoadSaved();
  static void SaveAll(const std::vector<std::pair<std::string, Grouping>> &saved);
  static void AddSaved(const std::string &name, const Grouping &grouping);
  static void RemoveSaved(const std::string &name);

 private:
  static std::string PrettyYearAlbum(int year, const std::string &album);
  static std::string PrettyAlbumDisc(const std::string &album, int disc);
  static std::string PrettyYearAlbumDisc(int year, const std::string &album, int disc);
  static std::string PrettyDisc(int disc);
  static std::string PrettyFormat(const Song &song);
  static std::string SortTextForName(const std::string &name, bool skip_articles);
  static std::string SortTextForNumber(int number);
  static std::string SortTextForYear(int year);
  static std::string SkipArticles(std::string name);
  static std::string NormalizeSort(std::string text);
  static Node *FindOrAddChild(Node *parent, const std::string &key, const std::string &display, const std::string &sort);
};

#endif  // STRAWBERRY_COLLECTIONGROUPING_H
