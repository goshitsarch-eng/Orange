#ifndef STRAWBERRY_FILTERPARSER_H
#define STRAWBERRY_FILTERPARSER_H

#include "core/song.h"

#include <string>

class FilterParser {
 public:
  explicit FilterParser(const std::string &filter);

  bool Matches(const Song &song) const;
  const std::string &filter() const { return filter_; }
  std::string ToSql() const;

 private:
  bool TermMatches(const std::string &term, const Song &song) const;

  std::string filter_;
};

#endif  // STRAWBERRY_FILTERPARSER_H
