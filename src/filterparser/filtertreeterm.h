#ifndef STRAWBERRY_FILTERTREETERM_H
#define STRAWBERRY_FILTERTREETERM_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include "filterparser/filtertree.h"

#include <memory>
#include <string>

class FilterTreeTerm : public FilterTree {
 public:
  explicit FilterTreeTerm(std::unique_ptr<FilterParserSearchTermComparator> comparator, std::string sql = "1=1");
  FilterType type() const override { return FilterType::Term; }
  bool accept(const Song &song) const override;
  std::string ToSql() const override { return sql_; }

 private:
  std::unique_ptr<FilterParserSearchTermComparator> cmp_;
  std::string sql_;
};

#endif
