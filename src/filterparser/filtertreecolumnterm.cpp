#include "filterparser/filtertreecolumnterm.h"

#include <cstdio>

FilterTreeColumnTerm::FilterTreeColumnTerm(FilterColumn column, std::unique_ptr<FilterParserSearchTermComparator> comparator,
                                           std::string sql, bool also_albumartist)
    : column_(column), cmp_(std::move(comparator)), sql_(std::move(sql)), also_albumartist_(also_albumartist) {}

bool FilterTreeColumnTerm::accept(const Song &song) const {
  if (!cmp_) {
    return true;
  }
  if (FilterTree::IsNumeric(column_)) {
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%.10g", FilterTree::NumericFromColumn(column_, song));
    return cmp_->Matches(buf);
  }
  if (cmp_->Matches(FilterTree::DataFromColumn(column_, song))) {
    return true;
  }
  return also_albumartist_ && cmp_->Matches(song.albumartist());
}
