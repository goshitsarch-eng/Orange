#ifndef STRAWBERRY_FILTERTREENOT_H
#define STRAWBERRY_FILTERTREENOT_H

#include "filterparser/filtertree.h"

#include <memory>

class FilterTreeNot : public FilterTree {
 public:
  explicit FilterTreeNot(std::unique_ptr<FilterTree> child);
  FilterType type() const override { return FilterType::Not; }
  bool accept(const Song &song) const override;

 private:
  std::unique_ptr<FilterTree> child_;
};

#endif
