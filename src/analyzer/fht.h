#ifndef STRAWBERRY_FHT_H
#define STRAWBERRY_FHT_H

#include <cstddef>
#include <vector>

class FHT {
 public:
  explicit FHT(int size = 64);
  void Transform(std::vector<float> *data) const;
  const std::vector<float> &window() const { return window_; }
  int size() const { return size_; }

 private:
  int size_ = 64;
  std::vector<float> window_;
};

#endif
