#ifndef STRAWBERRY_EBUR128ANALYSIS_H
#define STRAWBERRY_EBUR128ANALYSIS_H

#include "core/song.h"
#include "engine/ebur128measures.h"

#include <optional>

class EBUR128Analysis {
 public:
  EBUR128Analysis() = delete;
  static std::optional<EBUR128Measures> Compute(const Song &song);
};

#endif
