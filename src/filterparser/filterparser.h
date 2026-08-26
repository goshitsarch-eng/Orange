#ifndef STRAWBERRY_FILTERPARSER_H
#define STRAWBERRY_FILTERPARSER_H

#include "core/song.h"

#include <string>
#include <vector>

enum class FilterColumn {
  Unknown,
  Title,
  TitleSort,
  Album,
  AlbumSort,
  Artist,
  ArtistSort,
  AlbumArtist,
  AlbumArtistSort,
  Composer,
  ComposerSort,
  Performer,
  PerformerSort,
  Grouping,
  Genre,
  Comment,
  Filename,
  URL,
  Track,
  Year,
  Samplerate,
  Bitdepth,
  Bitrate,
  Playcount,
  Skipcount,
  Length,
  Rating,
  Age,
  Added,
  LastPlayed,
};

enum class FilterOperator {
  None,
  Eq,
  Ne,
  Gt,
  Ge,
  Lt,
  Le,
};

class FilterParser {
 public:
  explicit FilterParser(const std::string &filter);

  bool Matches(const Song &song) const;
  const std::string &filter() const { return filter_; }
  std::string ToSql() const;

 private:
  enum class NodeKind { And, Or, Not, Term };

  struct Node {
    NodeKind kind = NodeKind::And;
    FilterColumn column = FilterColumn::Unknown;
    FilterOperator op = FilterOperator::None;
    std::string value;
    std::vector<Node> children;
  };

  void Parse();
  Node ParseOr();
  Node ParseAnd();
  Node ParseUnary();
  Node ParseTerm();
  void SkipSpace();
  bool Consume(const std::string &word);
  bool MatchesNode(const Node &node, const Song &song) const;
  bool TermMatches(const Node &node, const Song &song) const;
  std::string NodeSql(const Node &node) const;
  std::string TermSql(const Node &node) const;
  static std::string ColumnSql(FilterColumn column);
  static FilterColumn ColumnFromName(const std::string &name);
  static bool IsNumeric(FilterColumn column);
  static bool IsTimeDays(FilterColumn column);
  static std::string TextValue(const Song &song, FilterColumn column);
  static double NumericValue(const Song &song, FilterColumn column);

  std::string filter_;
  size_t pos_ = 0;
  Node root_;
};

#endif
