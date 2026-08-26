#ifndef STRAWBERRY_FILTERTREEAND_H
#define STRAWBERRY_FILTERTREEAND_H

#include "filterparser/filtertree.h"

#include <memory>
#include <vector>

class FilterTreeAnd : public FilterTree {
 public:
  void Add(std::unique_ptr<FilterTree> child);
  FilterType type() const override { return FilterType::And; }
  bool accept(const Song &song) const override;

 private:
  std::vector<std::unique_ptr<FilterTree>> children_;
};

#endif
