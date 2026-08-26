#ifndef STRAWBERRY_MULTISORTFILTERPROXY_H
#define STRAWBERRY_MULTISORTFILTERPROXY_H

#include <algorithm>
#include <functional>
#include <vector>

template <typename T>
class MultiSortFilterProxy {
 public:
  using Compare = std::function<bool(const T &, const T &)>;
  void Sort(std::vector<T> *items, Compare compare) const {
    if (items && compare) {
      std::sort(items->begin(), items->end(), compare);
    }
  }
};

#endif
