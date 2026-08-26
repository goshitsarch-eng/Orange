#ifndef STRAWBERRY_EBUR128MEASURES_H
#define STRAWBERRY_EBUR128MEASURES_H

#include <optional>

struct EBUR128Measures {
  std::optional<double> loudness_lufs;
  std::optional<double> range_lu;
};

#endif
