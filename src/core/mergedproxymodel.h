#ifndef STRAWBERRY_MERGEDPROXYMODEL_H
#define STRAWBERRY_MERGEDPROXYMODEL_H

#include <cstddef>
#include <vector>

template <typename T>
class MergedProxyModel {
 public:
  void AddSource(const std::vector<T> *source) {
    if (source) {
      sources_.push_back(source);
    }
  }
  size_t Count() const {
    size_t n = 0;
    for (const auto *source : sources_) {
      n += source->size();
    }
    return n;
  }

 private:
  std::vector<const std::vector<T> *> sources_;
};

#endif
