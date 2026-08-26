#include "filterparser/filterparser.h"

#include "utilities/strutils.h"

#include <sstream>

FilterParser::FilterParser(const std::string &filter) : filter_(StrUtils::Trim(filter)) {}

bool FilterParser::TermMatches(const std::string &term, const Song &song) const {
  const auto colon = term.find(':');
  std::string field;
  std::string value = term;
  if (colon != std::string::npos) {
    field = StrUtils::ToLower(term.substr(0, colon));
    value = term.substr(colon + 1);
  }
  auto contains = [&value](const std::string &haystack) { return StrUtils::ContainsInsensitive(haystack, value); };
  if (field.empty()) {
    return contains(song.title()) || contains(song.album()) || contains(song.artist()) || contains(song.albumartist()) ||
           contains(song.composer()) || contains(song.genre()) || contains(song.comment());
  }
  if (field == "title") return contains(song.title());
  if (field == "album") return contains(song.album());
  if (field == "artist") return contains(song.artist()) || contains(song.albumartist());
  if (field == "albumartist") return contains(song.albumartist());
  if (field == "composer") return contains(song.composer());
  if (field == "performer") return contains(song.performer());
  if (field == "genre") return contains(song.genre());
  if (field == "comment") return contains(song.comment());
  if (field == "grouping") return contains(song.grouping());
  if (field == "year") return std::to_string(song.year()) == value;
  if (field == "rating") return song.rating() >= std::strtof(value.c_str(), nullptr);
  return contains(song.title());
}

bool FilterParser::Matches(const Song &song) const {
  if (filter_.empty()) {
    return true;
  }
  std::istringstream stream(filter_);
  std::string term;
  bool ok = true;
  while (stream >> term) {
    const bool negate = !term.empty() && term[0] == '-';
    if (negate) {
      term = term.substr(1);
    }
    const bool matches = TermMatches(term, song);
    ok = ok && (negate ? !matches : matches);
  }
  return ok;
}

std::string FilterParser::ToSql() const {
  if (filter_.empty()) {
    return {};
  }
  return "%" + StrUtils::SqlLikeEscape(filter_) + "%";
}
