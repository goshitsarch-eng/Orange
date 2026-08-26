#ifndef STRAWBERRY_FILTERTREENOP_H
#define STRAWBERRY_FILTERTREENOP_H

#include "filterparser/filtertree.h"

class FilterTreeNop : public FilterTree {
 public:
  FilterType type() const override { return FilterType::Nop; }
  bool accept(const Song &) const override { return true; }
};

#endif
