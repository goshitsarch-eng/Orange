#ifndef STRAWBERRY_FILTERTREETERM_H
#define STRAWBERRY_FILTERTREETERM_H

#include "filterparser/filterparsersearchtermcomparator.h"
#include "filterparser/filtertree.h"

#include <memory>
#include <string>

class FilterTreeTerm : public FilterTree {
 public:
  explicit FilterTreeTerm(std::unique_ptr<FilterParserSearchTermComparator> comparator);
  FilterType type() const override { return FilterType::Term; }
  bool accept(const Song &song) const override;

 private:
  std::unique_ptr<FilterParserSearchTermComparator> cmp_;
};

#endif
