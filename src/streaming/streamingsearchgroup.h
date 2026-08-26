#ifndef STRAWBERRY_STREAMINGSEARCHGROUP_H
#define STRAWBERRY_STREAMINGSEARCHGROUP_H

#include "collection/collectiongrouping.h"
#include "core/song.h"

#include <string>
#include <vector>

namespace StreamingSearchGroup {

constexpr char kSearchGroupBy1[] = "search_group_by1";
constexpr char kSearchGroupBy2[] = "search_group_by2";
constexpr char kSearchGroupBy3[] = "search_group_by3";

inline CollectionGrouping::Grouping DefaultGrouping() {
  using G = CollectionGrouping::GroupBy;
  return {G::AlbumArtist, G::AlbumDisc, G::None};
}

inline CollectionGrouping::Grouping FromSaved(int first, int second, int third, bool has_first) {
  if (!has_first) {
    return DefaultGrouping();
  }
  return {CollectionGrouping::FromInt(first), CollectionGrouping::FromInt(second), CollectionGrouping::FromInt(third)};
}

inline bool HasLevels(const CollectionGrouping::Grouping &grouping) {
  return grouping.first != CollectionGrouping::GroupBy::None;
}

struct Row {
  bool header = false;
  std::string label;
  int indent = 0;
  Song song;
};

inline void AppendFlattened(const CollectionGrouping::Node &node, int indent, std::vector<Row> *rows) {
  if (!rows) {
    return;
  }
  for (const CollectionGrouping::Node &child : node.children) {
    Row header;
    header.header = true;
    header.label = child.display;
    header.indent = indent;
    rows->push_back(header);
    AppendFlattened(child, indent + 1, rows);
  }
  for (const Song &song : node.songs) {
    Row row;
    row.indent = indent;
    row.song = song;
    rows->push_back(row);
  }
}

inline std::vector<Row> Flatten(const CollectionGrouping::Node &root) {
  std::vector<Row> rows;
  AppendFlattened(root, 0, &rows);
  return rows;
}

inline int HeaderCount(const std::vector<Row> &rows) {
  int count = 0;
  for (const Row &row : rows) {
    if (row.header) {
      ++count;
    }
  }
  return count;
}

inline int IndentPixels(int indent) { return 8 + indent * 16; }

inline int SongCount(const std::vector<Row> &rows) { return static_cast<int>(rows.size()) - HeaderCount(rows); }

inline std::vector<Row> RowsFor(const SongList &songs, const CollectionGrouping::Grouping &grouping, bool separate_albums_by_grouping = false) {
  if (!HasLevels(grouping)) {
    std::vector<Row> rows;
    rows.reserve(songs.size());
    for (const Song &song : songs) {
      Row row;
      row.song = song;
      rows.push_back(row);
    }
    return rows;
  }
  return Flatten(CollectionGrouping::BuildTree(songs, grouping, separate_albums_by_grouping, true, false));
}

}  // namespace StreamingSearchGroup

#endif
