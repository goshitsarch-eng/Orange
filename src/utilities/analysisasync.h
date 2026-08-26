#ifndef STRAWBERRY_ANALYSISASYNC_H
#define STRAWBERRY_ANALYSISASYNC_H

namespace AnalysisAsync {

inline bool NeedsGenerate(bool enabled, bool cache_hit) { return enabled && !cache_hit; }

inline bool AcceptGeneration(int job_generation, int current_generation, bool alive) {
  return alive && job_generation == current_generation;
}

}  // namespace AnalysisAsync

#endif
