#ifndef STRAWBERRY_COVERFETCHPOLICY_H
#define STRAWBERRY_COVERFETCHPOLICY_H

#include "covermanager/albumcoverfetchersearch.h"

namespace CoverFetchPolicy {

inline bool ShouldStop(float best_score, bool search, float good_score = AlbumCoverFetcherSearch::kGoodScore) {
  return !search && best_score >= good_score;
}

}  // namespace CoverFetchPolicy

#endif  // STRAWBERRY_COVERFETCHPOLICY_H
