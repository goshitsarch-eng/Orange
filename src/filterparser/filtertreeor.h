#ifndef STRAWBERRY_FILTERTREEOR_H
#define STRAWBERRY_FILTERTREEOR_H

#include "filterparser/filtertree.h"

#include <memory>
#include <vector>

class FilterTreeOr : public FilterTree {
 public:
  void Add(std::unique_ptr<FilterTree> child);
  FilterType type() const override { return FilterType::Or; }
  bool accept(const Song &song) const override;

 private:
  std::vector<std::unique_ptr<FilterTree>> children_;
};

#endif
