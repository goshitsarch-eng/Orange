#include "filterparser/filtertreeor.h"

void FilterTreeOr::Add(std::unique_ptr<FilterTree> child) {
  if (child) {
    children_.push_back(std::move(child));
  }
}

bool FilterTreeOr::accept(const Song &song) const {
  if (children_.empty()) {
    return true;
  }
  for (const auto &child : children_) {
    if (child && child->accept(song)) {
      return true;
    }
  }
  return false;
}
