#include "analyzer/fht.h"

#include <cmath>

FHT::FHT(int size) : size_(size), window_(static_cast<size_t>(size), 1.0f) {
  for (int i = 0; i < size_; ++i) {
    window_[static_cast<size_t>(i)] = static_cast<float>(0.5 - 0.5 * std::cos(2.0 * M_PI * i / size_));
  }
}

void FHT::Transform(std::vector<float> *data) const {
  if (!data || data->empty()) {
    return;
  }
  const size_t n = std::min(data->size(), window_.size());
  for (size_t i = 0; i < n; ++i) {
    (*data)[i] *= window_[i];
  }
}
