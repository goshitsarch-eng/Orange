#include "filterparser/filterparser.h"

#include "filterparser/filterparsersearchcomparators.h"
#include "filterparser/filtertreeand.h"
#include "filterparser/filtertreecolumnterm.h"
#include "filterparser/filtertreenop.h"
#include "filterparser/filtertreenot.h"
#include "filterparser/filtertreeor.h"
#include "filterparser/filtertreeterm.h"
#include "utilities/strutils.h"

#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <memory>
#include <vector>

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

FilterOperator FilterParser::OperatorFromPrefix(const std::string &prefix) {
  if (prefix == "=" || prefix == "==") return FilterOperator::Eq;
  if (prefix == "!=" || prefix == "<>") return FilterOperator::Ne;
  if (prefix == ">") return FilterOperator::Gt;
  if (prefix == ">=") return FilterOperator::Ge;
  if (prefix == "<") return FilterOperator::Lt;
  if (prefix == "<=") return FilterOperator::Le;
  return FilterOperator::None;
}

std::string FilterParser::FreeTextSql(const std::string &value) {
  if (value.empty()) {
    return "1=1";
  }
  const std::string like = "'%" + StrUtils::SqlLikeEscape(value) + "%' ESCAPE '\\'";
  return "(title LIKE " + like + " OR album LIKE " + like + " OR artist LIKE " + like + " OR albumartist LIKE " + like +
         " OR composer LIKE " + like + " OR performer LIKE " + like + " OR genre LIKE " + like + " OR comment LIKE " + like + ")";
}

std::string FilterParser::TermSql(FilterColumn column, FilterOperator op, const std::string &value) {
  const std::string sql_column = ColumnSql(column);
  if (sql_column.empty()) {
    return "1=1";
  }
  if (IsTimeDays(column)) {
    const int days = std::atoi(value.c_str());
    const std::string cutoff = "strftime('%s','now') - " + std::to_string(static_cast<int64_t>(days) * 86400);
    std::string sql_op = ">=";
    if (op == FilterOperator::Lt) sql_op = "<";
    else if (op == FilterOperator::Le) sql_op = "<=";
    else if (op == FilterOperator::Gt) sql_op = ">";
    else if (op == FilterOperator::Ge) sql_op = ">=";
    else if (op == FilterOperator::Eq) sql_op = "=";
    else if (op == FilterOperator::Ne) sql_op = "!=";
    return "(" + sql_column + " > 0 AND " + sql_column + " " + sql_op + " " + cutoff + ")";
  }
  if (IsNumeric(column)) {
    std::string expr = sql_column;
    if (column == FilterColumn::Rating) {
      expr = "(rating / 100.0)";
    }
    std::string sql_op = "=";
    if (op == FilterOperator::None && (column == FilterColumn::Rating || column == FilterColumn::Playcount || column == FilterColumn::Skipcount)) {
      sql_op = ">=";
    } else if (op == FilterOperator::Ne) {
      sql_op = "!=";
    } else if (op == FilterOperator::Gt) {
      sql_op = ">";
    } else if (op == FilterOperator::Ge) {
      sql_op = ">=";
    } else if (op == FilterOperator::Lt) {
      sql_op = "<";
    } else if (op == FilterOperator::Le) {
      sql_op = "<=";
    }
    return expr + " " + sql_op + " " + value;
  }
  if (op == FilterOperator::Eq) {
    return "LOWER(" + sql_column + ") = " + StrUtils::SqlQuote(StrUtils::ToLower(value));
  }
  if (op == FilterOperator::Ne) {
    return "LOWER(" + sql_column + ") != " + StrUtils::SqlQuote(StrUtils::ToLower(value));
  }
  return sql_column + " LIKE '%" + StrUtils::SqlLikeEscape(value) + "%' ESCAPE '\\'";
}

namespace {

std::unique_ptr<FilterParserSearchTermComparator> MakeIntComparator(FilterOperator op, int value) {
  switch (op) {
    case FilterOperator::Ne:
      return std::make_unique<FilterParserIntNeComparator>(value);
    case FilterOperator::Gt:
      return std::make_unique<FilterParserIntGtComparator>(value);
    case FilterOperator::Ge:
      return std::make_unique<FilterParserIntGeComparator>(value);
    case FilterOperator::Lt:
      return std::make_unique<FilterParserIntLtComparator>(value);
    case FilterOperator::Le:
      return std::make_unique<FilterParserIntLeComparator>(value);
    case FilterOperator::Eq:
    case FilterOperator::None:
    default:
      return std::make_unique<FilterParserIntEqComparator>(value);
  }
}

std::unique_ptr<FilterParserSearchTermComparator> MakeInt64Comparator(FilterOperator op, int64_t value) {
  switch (op) {
    case FilterOperator::Ne:
      return std::make_unique<FilterParserInt64NeComparator>(value);
    case FilterOperator::Gt:
      return std::make_unique<FilterParserInt64GtComparator>(value);
    case FilterOperator::Ge:
      return std::make_unique<FilterParserInt64GeComparator>(value);
    case FilterOperator::Lt:
      return std::make_unique<FilterParserInt64LtComparator>(value);
    case FilterOperator::Le:
      return std::make_unique<FilterParserInt64LeComparator>(value);
    case FilterOperator::Eq:
    case FilterOperator::None:
    default:
      return std::make_unique<FilterParserInt64EqComparator>(value);
  }
}

std::unique_ptr<FilterParserSearchTermComparator> MakeFloatComparator(FilterOperator op, double value) {
  switch (op) {
    case FilterOperator::Ne:
      return std::make_unique<FilterParserFloatNeComparator>(value);
    case FilterOperator::Gt:
      return std::make_unique<FilterParserFloatGtComparator>(value);
    case FilterOperator::Ge:
      return std::make_unique<FilterParserFloatGeComparator>(value);
    case FilterOperator::Lt:
      return std::make_unique<FilterParserFloatLtComparator>(value);
    case FilterOperator::Le:
      return std::make_unique<FilterParserFloatLeComparator>(value);
    case FilterOperator::Eq:
    case FilterOperator::None:
    default:
      return std::make_unique<FilterParserFloatEqComparator>(value);
  }
}

}  // namespace

std::unique_ptr<FilterTree> FilterParser::CreateSearchTerm(const std::string &column_name, const std::string &prefix,
                                                           const std::string &value) const {
  const FilterOperator op = OperatorFromPrefix(prefix);
  const FilterColumn column = ColumnFromName(column_name);
  if (column == FilterColumn::Unknown && value.empty()) {
    return std::make_unique<FilterTreeNop>();
  }
  if (column == FilterColumn::Unknown) {
    return std::make_unique<FilterTreeTerm>(std::make_unique<FilterParserTextContainsComparator>(value), FreeTextSql(value));
  }

  std::unique_ptr<FilterParserSearchTermComparator> cmp;
  if (IsTimeDays(column)) {
    const int days = std::atoi(value.c_str());
    const int64_t cutoff = static_cast<int64_t>(std::time(nullptr)) - static_cast<int64_t>(days) * 86400;
    cmp = MakeInt64Comparator(op == FilterOperator::None ? FilterOperator::Ge : op, cutoff);
  } else if (column == FilterColumn::Rating) {
    cmp = MakeFloatComparator(op == FilterOperator::None ? FilterOperator::Ge : op, std::strtod(value.c_str(), nullptr));
  } else if (column == FilterColumn::Length) {
    cmp = MakeInt64Comparator(op == FilterOperator::None ? FilterOperator::Eq : op, std::strtoll(value.c_str(), nullptr, 10));
  } else if (IsNumeric(column)) {
    FilterOperator effective = op;
    if (op == FilterOperator::None && (column == FilterColumn::Playcount || column == FilterColumn::Skipcount)) {
      effective = FilterOperator::Ge;
    }
    cmp = MakeIntComparator(effective, static_cast<int>(std::strtol(value.c_str(), nullptr, 10)));
  } else if (op == FilterOperator::Eq) {
    cmp = std::make_unique<FilterParserTextEqComparator>(value);
  } else if (op == FilterOperator::Ne) {
    cmp = std::make_unique<FilterParserTextNeComparator>(value);
  } else {
    cmp = std::make_unique<FilterParserTextContainsComparator>(value);
  }
  const bool also_albumartist = column == FilterColumn::Artist && op == FilterOperator::None;
  return std::make_unique<FilterTreeColumnTerm>(column, std::move(cmp), TermSql(column, op, value), also_albumartist);
}

void FilterParser::Parse() {
  pos_ = 0;
  if (filter_.empty()) {
    tree_ = std::make_unique<FilterTreeNop>();
    return;
  }
  tree_ = ParseOr();
}

std::unique_ptr<FilterTree> FilterParser::ParseOr() {
  auto node = ParseAnd();
  SkipSpace();
  if (!Consume("OR")) {
    return node;
  }
  auto group = std::make_unique<FilterTreeOr>();
  group->Add(std::move(node));
  do {
    group->Add(ParseAnd());
  } while (Consume("OR"));
  return group;
}

std::unique_ptr<FilterTree> FilterParser::ParseAnd() {
  std::vector<std::unique_ptr<FilterTree>> children;
  children.push_back(ParseUnary());
  SkipSpace();
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
    return std::move(children.front());
  }
  auto group = std::make_unique<FilterTreeAnd>();
  for (auto &child : children) {
    group->Add(std::move(child));
  }
  return group;
}

std::unique_ptr<FilterTree> FilterParser::ParseUnary() {
  SkipSpace();
  if (pos_ < filter_.size() && filter_[pos_] == '(') {
    ++pos_;
    auto inner = ParseOr();
    SkipSpace();
    if (pos_ < filter_.size() && filter_[pos_] == ')') {
      ++pos_;
    }
    return inner;
  }
  if (pos_ < filter_.size() && filter_[pos_] == '-') {
    ++pos_;
    return std::make_unique<FilterTreeNot>(ParseUnary());
  }
  return ParseTerm();
}

std::unique_ptr<FilterTree> FilterParser::ParseTerm() {
  SkipSpace();
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
    if (value.empty() && !column.empty() && prefix.empty() && (ch == '>' || ch == '<' || ch == '=' || ch == '!')) {
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
  return CreateSearchTerm(column, prefix, value);
}

bool FilterParser::Matches(const Song &song) const {
  if (filter_.empty()) {
    return true;
  }
  return tree_ ? tree_->accept(song) : true;
}

std::string FilterParser::ToSql() const {
  if (filter_.empty()) {
    return {};
  }
  return tree_ ? tree_->ToSql() : std::string();
}

std::string FilterParser::ToolTip() {
  return "Prefix a search term with a field name to limit the search, e.g. artist:Strawbs. "
         "Exclude a word with a preceding -. Numerical fields accept =, !=, <, >, <= and >=. "
         "Combine terms with AND (default) or OR, and group with parentheses. "
         "Available fields: title, album, artist, albumartist, composer, performer, grouping, genre, "
         "comment, filename, url, track, year, samplerate, bitdepth, bitrate, playcount, skipcount, "
         "length, rating, age, added, lastplayed.";
}
