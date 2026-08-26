#include "filterparser/filtertreecolumnterm.h"

#include <cstdio>

FilterTreeColumnTerm::FilterTreeColumnTerm(FilterColumn column, std::unique_ptr<FilterParserSearchTermComparator> comparator)
    : column_(column), cmp_(std::move(comparator)) {}

bool FilterTreeColumnTerm::accept(const Song &song) const {
  if (!cmp_) {
    return true;
  }
  if (FilterTree::IsNumeric(column_)) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.10g", FilterTree::NumericFromColumn(column_, song));
    return cmp_->Matches(buf);
  }
  return cmp_->Matches(FilterTree::DataFromColumn(column_, song));
}
