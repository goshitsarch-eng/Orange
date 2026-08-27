#ifndef STRAWBERRY_FILTERTREECOLUMNTERM_H
#define STRAWBERRY_FILTERTREECOLUMNTERM_H

#include "filterparser/filtercolumn.h"
#include "filterparser/filterparsersearchtermcomparator.h"
#include "filterparser/filtertree.h"

#include <memory>
#include <string>

class FilterTreeColumnTerm : public FilterTree {
 public:
  FilterTreeColumnTerm(FilterColumn column, std::unique_ptr<FilterParserSearchTermComparator> comparator, std::string sql = "1=1",
                       bool also_albumartist = false);
  FilterType type() const override { return FilterType::Column; }
  bool accept(const Song &song) const override;
  std::string ToSql() const override { return sql_; }

 private:
  FilterColumn column_ = FilterColumn::Unknown;
  std::unique_ptr<FilterParserSearchTermComparator> cmp_;
  std::string sql_;
  bool also_albumartist_ = false;
};

#endif
