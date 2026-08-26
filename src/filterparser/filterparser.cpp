#include "filterparser/filterparser.h"

#include "utilities/fileutils.h"
#include "utilities/strutils.h"

#include <cctype>
#include <cstdlib>
#include <ctime>

namespace {

bool CompareNumber(double left, FilterOperator op, double right) {
  switch (op) {
    case FilterOperator::Ne:
      return left != right;
    case FilterOperator::Gt:
      return left > right;
    case FilterOperator::Ge:
      return left >= right;
    case FilterOperator::Lt:
      return left < right;
    case FilterOperator::Le:
      return left <= right;
    case FilterOperator::Eq:
    case FilterOperator::None:
    default:
      return left == right;
  }
}

}  // namespace

FilterParser::FilterParser(const std::string &filter) : filter_(StrUtils::Trim(filter)) {
  Parse();
}

void FilterParser::SkipSpace() {
  while (pos_ < filter_.size() && std::isspace(static_cast<unsigned char>(filter_[pos_]))) {
    ++pos_;
  }
}

bool FilterParser::Consume(const std::string &word) {
  SkipSpace();
  if (pos_ + word.size() > filter_.size()) {
    return false;
  }
  for (size_t i = 0; i < word.size(); ++i) {
    if (std::toupper(static_cast<unsigned char>(filter_[pos_ + i])) != std::toupper(static_cast<unsigned char>(word[i]))) {
      return false;
    }
  }
  const size_t after = pos_ + word.size();
  if (after < filter_.size() && std::isalnum(static_cast<unsigned char>(filter_[after]))) {
    return false;
  }
  pos_ = after;
  return true;
}

FilterColumn FilterParser::ColumnFromName(const std::string &name) {
  const std::string key = StrUtils::ToLower(name);
  if (key == "title") return FilterColumn::Title;
  if (key == "titlesort") return FilterColumn::TitleSort;
  if (key == "album") return FilterColumn::Album;
  if (key == "albumsort") return FilterColumn::AlbumSort;
  if (key == "artist") return FilterColumn::Artist;
  if (key == "artistsort") return FilterColumn::ArtistSort;
  if (key == "albumartist") return FilterColumn::AlbumArtist;
  if (key == "albumartistsort") return FilterColumn::AlbumArtistSort;
  if (key == "composer") return FilterColumn::Composer;
  if (key == "composersort") return FilterColumn::ComposerSort;
  if (key == "performer") return FilterColumn::Performer;
  if (key == "performersort") return FilterColumn::PerformerSort;
  if (key == "grouping") return FilterColumn::Grouping;
  if (key == "genre") return FilterColumn::Genre;
  if (key == "comment") return FilterColumn::Comment;
  if (key == "filename" || key == "file") return FilterColumn::Filename;
  if (key == "url") return FilterColumn::URL;
  if (key == "track") return FilterColumn::Track;
  if (key == "year") return FilterColumn::Year;
  if (key == "samplerate") return FilterColumn::Samplerate;
  if (key == "bitdepth") return FilterColumn::Bitdepth;
  if (key == "bitrate") return FilterColumn::Bitrate;
  if (key == "playcount") return FilterColumn::Playcount;
  if (key == "skipcount") return FilterColumn::Skipcount;
  if (key == "length") return FilterColumn::Length;
  if (key == "rating") return FilterColumn::Rating;
  if (key == "age") return FilterColumn::Age;
  if (key == "added") return FilterColumn::Added;
  if (key == "lastplayed") return FilterColumn::LastPlayed;
  return FilterColumn::Unknown;
}

bool FilterParser::IsNumeric(FilterColumn column) {
  switch (column) {
    case FilterColumn::Track:
    case FilterColumn::Year:
    case FilterColumn::Samplerate:
    case FilterColumn::Bitdepth:
    case FilterColumn::Bitrate:
    case FilterColumn::Playcount:
    case FilterColumn::Skipcount:
    case FilterColumn::Length:
    case FilterColumn::Rating:
    case FilterColumn::Age:
    case FilterColumn::Added:
    case FilterColumn::LastPlayed:
      return true;
    default:
      return false;
  }
}

bool FilterParser::IsTimeDays(FilterColumn column) {
  return column == FilterColumn::Age || column == FilterColumn::Added || column == FilterColumn::LastPlayed;
}

std::string FilterParser::ColumnSql(FilterColumn column) {
  switch (column) {
    case FilterColumn::Title:
      return "title";
    case FilterColumn::TitleSort:
      return "titlesort";
    case FilterColumn::Album:
      return "album";
    case FilterColumn::AlbumSort:
      return "albumsort";
    case FilterColumn::Artist:
      return "artist";
    case FilterColumn::ArtistSort:
      return "artistsort";
    case FilterColumn::AlbumArtist:
      return "albumartist";
    case FilterColumn::AlbumArtistSort:
      return "albumartistsort";
    case FilterColumn::Composer:
      return "composer";
    case FilterColumn::ComposerSort:
      return "composersort";
    case FilterColumn::Performer:
      return "performer";
    case FilterColumn::PerformerSort:
      return "performersort";
    case FilterColumn::Grouping:
      return "grouping";
    case FilterColumn::Genre:
      return "genre";
    case FilterColumn::Comment:
      return "comment";
    case FilterColumn::Filename:
    case FilterColumn::URL:
      return "url";
    case FilterColumn::Track:
      return "track";
    case FilterColumn::Year:
      return "year";
    case FilterColumn::Samplerate:
      return "samplerate";
    case FilterColumn::Bitdepth:
      return "bitdepth";
    case FilterColumn::Bitrate:
      return "bitrate";
    case FilterColumn::Playcount:
      return "playcount";
    case FilterColumn::Skipcount:
      return "skipcount";
    case FilterColumn::Length:
      return "length";
    case FilterColumn::Rating:
      return "rating";
    case FilterColumn::Age:
    case FilterColumn::Added:
      return "ctime";
    case FilterColumn::LastPlayed:
      return "lastplayed";
    default:
      return {};
  }
}

std::string FilterParser::TextValue(const Song &song, FilterColumn column) {
  switch (column) {
    case FilterColumn::Title:
      return song.title();
    case FilterColumn::TitleSort:
      return song.titlesort();
    case FilterColumn::Album:
      return song.album();
    case FilterColumn::AlbumSort:
      return song.albumsort();
    case FilterColumn::Artist:
      return song.artist();
    case FilterColumn::ArtistSort:
      return song.artistsort();
    case FilterColumn::AlbumArtist:
      return song.albumartist();
    case FilterColumn::AlbumArtistSort:
      return song.albumartistsort();
    case FilterColumn::Composer:
      return song.composer();
    case FilterColumn::ComposerSort:
      return song.composersort();
    case FilterColumn::Performer:
      return song.performer();
    case FilterColumn::PerformerSort:
      return song.performersort();
    case FilterColumn::Grouping:
      return song.grouping();
    case FilterColumn::Genre:
      return song.genre();
    case FilterColumn::Comment:
      return song.comment();
    case FilterColumn::Filename:
      return song.basefilename().empty() ? FileUtils::BaseName(FileUtils::PathFromUri(song.url())) : song.basefilename();
    case FilterColumn::URL:
      return song.url();
    default:
      return {};
  }
}

double FilterParser::NumericValue(const Song &song, FilterColumn column) {
  switch (column) {
    case FilterColumn::Track:
      return song.track();
    case FilterColumn::Year:
      return song.year();
    case FilterColumn::Samplerate:
      return song.samplerate();
    case FilterColumn::Bitdepth:
      return song.bitdepth();
    case FilterColumn::Bitrate:
      return song.bitrate();
    case FilterColumn::Playcount:
      return song.playcount();
    case FilterColumn::Skipcount:
      return song.skipcount();
    case FilterColumn::Length:
      return static_cast<double>(song.length_nanosec());
    case FilterColumn::Rating:
      return song.rating();
    case FilterColumn::Age:
    case FilterColumn::Added:
      return static_cast<double>(song.ctime());
    case FilterColumn::LastPlayed:
      return static_cast<double>(song.lastplayed());
    default:
      return 0;
  }
}

void FilterParser::Parse() {
  pos_ = 0;
  if (filter_.empty()) {
    root_ = Node{};
    return;
  }
  root_ = ParseOr();
}

FilterParser::Node FilterParser::ParseOr() {
  Node node = ParseAnd();
  SkipSpace();
  if (!Consume("OR")) {
    return node;
  }
  Node group;
  group.kind = NodeKind::Or;
  group.children.push_back(std::move(node));
  do {
    group.children.push_back(ParseAnd());
  } while (Consume("OR"));
  return group;
}

FilterParser::Node FilterParser::ParseAnd() {
  Node node = ParseUnary();
  SkipSpace();
  std::vector<Node> children;
  children.push_back(std::move(node));
  while (pos_ < filter_.size() && filter_[pos_] != ')') {
    if (Consume("OR")) {
      pos_ -= 2;
      break;
    }
    Consume("AND");
    SkipSpace();
    if (pos_ >= filter_.size() || filter_[pos_] == ')') {
      break;
    }
    children.push_back(ParseUnary());
    SkipSpace();
  }
  if (children.size() == 1) {
    return children.front();
  }
  Node group;
  group.kind = NodeKind::And;
  group.children = std::move(children);
  return group;
}

FilterParser::Node FilterParser::ParseUnary() {
  SkipSpace();
  if (pos_ < filter_.size() && filter_[pos_] == '(') {
    ++pos_;
    Node inner = ParseOr();
    SkipSpace();
    if (pos_ < filter_.size() && filter_[pos_] == ')') {
      ++pos_;
    }
    return inner;
  }
  if (pos_ < filter_.size() && filter_[pos_] == '-') {
    ++pos_;
    Node node;
    node.kind = NodeKind::Not;
    node.children.push_back(ParseUnary());
    return node;
  }
  return ParseTerm();
}

FilterParser::Node FilterParser::ParseTerm() {
  SkipSpace();
  Node node;
  node.kind = NodeKind::Term;
  std::string column;
  std::string prefix;
  std::string value;
  bool in_quotes = false;
  while (pos_ < filter_.size()) {
    const char ch = filter_[pos_];
    if (in_quotes) {
      if (ch == '"') {
        in_quotes = false;
        ++pos_;
      } else {
        value.push_back(ch);
        ++pos_;
      }
      continue;
    }
    if (ch == '"') {
      in_quotes = true;
      ++pos_;
      continue;
    }
    if (column.empty() && ch == ':') {
      column = StrUtils::ToLower(value);
      value.clear();
      ++pos_;
      while (pos_ < filter_.size() && std::isspace(static_cast<unsigned char>(filter_[pos_]))) {
        ++pos_;
      }
      continue;
    }
    if (std::isspace(static_cast<unsigned char>(ch)) || ch == '(' || ch == ')' || ch == '-') {
      break;
    }
    if (value.empty() && column.empty() == false && prefix.empty() && (ch == '>' || ch == '<' || ch == '=' || ch == '!')) {
      prefix.push_back(ch);
      ++pos_;
      if (pos_ < filter_.size() && filter_[pos_] == '=' && prefix != "=") {
        prefix.push_back('=');
        ++pos_;
      }
      continue;
    }
    if (value.empty() && column.empty() && (ch == '>' || ch == '<' || ch == '=' || ch == '!')) {
      prefix.push_back(ch);
      ++pos_;
      if (pos_ < filter_.size() && filter_[pos_] == '=' && prefix != "=") {
        prefix.push_back('=');
        ++pos_;
      }
      continue;
    }
    if (column.empty() && (ch == '>' || ch == '<' || ch == '=' || ch == '!') && !value.empty()) {
      column = StrUtils::ToLower(value);
      value.clear();
      prefix.push_back(ch);
      ++pos_;
      if (pos_ < filter_.size() && filter_[pos_] == '=' && prefix != "=") {
        prefix.push_back('=');
        ++pos_;
      }
      continue;
    }
    value.push_back(ch);
    ++pos_;
  }

  if (prefix == "=" || prefix == "==") {
    node.op = FilterOperator::Eq;
  } else if (prefix == "!=" || prefix == "<>") {
    node.op = FilterOperator::Ne;
  } else if (prefix == ">") {
    node.op = FilterOperator::Gt;
  } else if (prefix == ">=") {
    node.op = FilterOperator::Ge;
  } else if (prefix == "<") {
    node.op = FilterOperator::Lt;
  } else if (prefix == "<=") {
    node.op = FilterOperator::Le;
  }
  node.column = ColumnFromName(column);
  node.value = value;
  return node;
}

bool FilterParser::TermMatches(const Node &node, const Song &song) const {
  if (node.column == FilterColumn::Unknown && node.value.empty()) {
    return true;
  }
  if (node.column == FilterColumn::Unknown) {
    return StrUtils::ContainsInsensitive(song.title(), node.value) || StrUtils::ContainsInsensitive(song.album(), node.value) ||
           StrUtils::ContainsInsensitive(song.artist(), node.value) || StrUtils::ContainsInsensitive(song.albumartist(), node.value) ||
           StrUtils::ContainsInsensitive(song.composer(), node.value) || StrUtils::ContainsInsensitive(song.performer(), node.value) ||
           StrUtils::ContainsInsensitive(song.genre(), node.value) || StrUtils::ContainsInsensitive(song.comment(), node.value);
  }
  if (IsTimeDays(node.column)) {
    const int64_t stamp = node.column == FilterColumn::LastPlayed ? song.lastplayed() : song.ctime();
    if (stamp <= 0) {
      return false;
    }
    const int days = std::atoi(node.value.c_str());
    const int64_t cutoff = static_cast<int64_t>(std::time(nullptr)) - static_cast<int64_t>(days) * 86400;
    const double left = static_cast<double>(stamp);
    const double right = static_cast<double>(cutoff);
    if (node.op == FilterOperator::None) {
      return stamp >= cutoff || days <= 0;
    }
    return CompareNumber(left, node.op, right);
  }
  if (IsNumeric(node.column)) {
    const double left = NumericValue(song, node.column);
    const double right = std::strtod(node.value.c_str(), nullptr);
    if (node.op == FilterOperator::None && (node.column == FilterColumn::Rating || node.column == FilterColumn::Playcount ||
                                            node.column == FilterColumn::Skipcount)) {
      return left >= right;
    }
    if (node.op == FilterOperator::None) {
      return left == right;
    }
    return CompareNumber(left, node.op, right);
  }
  const std::string text = TextValue(song, node.column);
  if (node.column == FilterColumn::Artist && node.op == FilterOperator::None) {
    return StrUtils::ContainsInsensitive(song.artist(), node.value) || StrUtils::ContainsInsensitive(song.albumartist(), node.value);
  }
  if (node.op == FilterOperator::Eq) {
    return StrUtils::ToLower(text) == StrUtils::ToLower(node.value);
  }
  if (node.op == FilterOperator::Ne) {
    return StrUtils::ToLower(text) != StrUtils::ToLower(node.value);
  }
  return StrUtils::ContainsInsensitive(text, node.value);
}

bool FilterParser::MatchesNode(const Node &node, const Song &song) const {
  switch (node.kind) {
    case NodeKind::Not:
      return node.children.empty() ? true : !MatchesNode(node.children.front(), song);
    case NodeKind::Or: {
      for (const Node &child : node.children) {
        if (MatchesNode(child, song)) {
          return true;
        }
      }
      return node.children.empty();
    }
    case NodeKind::And: {
      for (const Node &child : node.children) {
        if (!MatchesNode(child, song)) {
          return false;
        }
      }
      return true;
    }
    case NodeKind::Term:
    default:
      return TermMatches(node, song);
  }
}

bool FilterParser::Matches(const Song &song) const {
  if (filter_.empty()) {
    return true;
  }
  return MatchesNode(root_, song);
}

std::string FilterParser::TermSql(const Node &node) const {
  if (node.column == FilterColumn::Unknown) {
    if (node.value.empty()) {
      return "1=1";
    }
    const std::string like = "'%" + StrUtils::SqlLikeEscape(node.value) + "%' ESCAPE '\\'";
    return "(title LIKE " + like + " OR album LIKE " + like + " OR artist LIKE " + like + " OR albumartist LIKE " + like +
           " OR composer LIKE " + like + " OR performer LIKE " + like + " OR genre LIKE " + like + " OR comment LIKE " + like + ")";
  }
  const std::string column = ColumnSql(node.column);
  if (column.empty()) {
    return "1=1";
  }
  if (IsTimeDays(node.column)) {
    const int days = std::atoi(node.value.c_str());
    const std::string cutoff = "strftime('%s','now') - " + std::to_string(static_cast<int64_t>(days) * 86400);
    std::string op = ">=";
    if (node.op == FilterOperator::Lt) op = "<";
    else if (node.op == FilterOperator::Le) op = "<=";
    else if (node.op == FilterOperator::Gt) op = ">";
    else if (node.op == FilterOperator::Ge) op = ">=";
    else if (node.op == FilterOperator::Eq) op = "=";
    else if (node.op == FilterOperator::Ne) op = "!=";
    return "(" + column + " > 0 AND " + column + " " + op + " " + cutoff + ")";
  }
  if (IsNumeric(node.column)) {
    std::string sql_column = column;
    std::string value = node.value;
    if (node.column == FilterColumn::Rating) {
      sql_column = "(rating / 100.0)";
    }
    std::string op = "=";
    if (node.op == FilterOperator::None && (node.column == FilterColumn::Rating || node.column == FilterColumn::Playcount ||
                                            node.column == FilterColumn::Skipcount)) {
      op = ">=";
    } else if (node.op == FilterOperator::Ne) {
      op = "!=";
    } else if (node.op == FilterOperator::Gt) {
      op = ">";
    } else if (node.op == FilterOperator::Ge) {
      op = ">=";
    } else if (node.op == FilterOperator::Lt) {
      op = "<";
    } else if (node.op == FilterOperator::Le) {
      op = "<=";
    }
    return sql_column + " " + op + " " + value;
  }
  if (node.op == FilterOperator::Eq) {
    return "LOWER(" + column + ") = " + StrUtils::SqlQuote(StrUtils::ToLower(node.value));
  }
  if (node.op == FilterOperator::Ne) {
    return "LOWER(" + column + ") != " + StrUtils::SqlQuote(StrUtils::ToLower(node.value));
  }
  return column + " LIKE '%" + StrUtils::SqlLikeEscape(node.value) + "%' ESCAPE '\\'";
}

std::string FilterParser::NodeSql(const Node &node) const {
  switch (node.kind) {
    case NodeKind::Not:
      return node.children.empty() ? "1=1" : "(NOT " + NodeSql(node.children.front()) + ")";
    case NodeKind::Or: {
      if (node.children.empty()) {
        return "1=1";
      }
      std::string sql = "(";
      for (size_t i = 0; i < node.children.size(); ++i) {
        if (i) {
          sql += " OR ";
        }
        sql += NodeSql(node.children[i]);
      }
      sql += ")";
      return sql;
    }
    case NodeKind::And: {
      if (node.children.empty()) {
        return "1=1";
      }
      std::string sql = "(";
      for (size_t i = 0; i < node.children.size(); ++i) {
        if (i) {
          sql += " AND ";
        }
        sql += NodeSql(node.children[i]);
      }
      sql += ")";
      return sql;
    }
    case NodeKind::Term:
    default:
      return TermSql(node);
  }
}

std::string FilterParser::ToSql() const {
  if (filter_.empty()) {
    return {};
  }
  return NodeSql(root_);
}
