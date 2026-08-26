#include "filterparser/filtertreeand.h"

void FilterTreeAnd::Add(std::unique_ptr<FilterTree> child) {
  if (child) {
    children_.push_back(std::move(child));
  }
}

bool FilterTreeAnd::accept(const Song &song) const {
  for (const auto &child : children_) {
    if (child && !child->accept(song)) {
      return false;
    }
  }
  return true;
}
