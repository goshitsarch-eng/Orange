#ifndef STRAWBERRY_FILTERTREECOLUMNTERM_H
#define STRAWBERRY_FILTERTREECOLUMNTERM_H

#include "filterparser/filtercolumn.h"
#include "filterparser/filterparsersearchtermcomparator.h"
#include "filterparser/filtertree.h"

#include <memory>

class FilterTreeColumnTerm : public FilterTree {
 public:
  FilterTreeColumnTerm(FilterColumn column, std::unique_ptr<FilterParserSearchTermComparator> comparator);
  FilterType type() const override { return FilterType::Column; }
  bool accept(const Song &song) const override;

 private:
  FilterColumn column_ = FilterColumn::Unknown;
  std::unique_ptr<FilterParserSearchTermComparator> cmp_;
};

#endif
