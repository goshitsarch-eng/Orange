#ifndef STRAWBERRY_ENGINEFADEOUT_H
#define STRAWBERRY_ENGINEFADEOUT_H

#include <algorithm>

namespace EngineFadeout {

inline constexpr int kTickMs = 50;

// Qt StartFadeout returns immediately when that pipeline is already fading.
inline bool ShouldInsert(int pipeline_id, bool already_contains) { return pipeline_id != 0 && !already_contains; }

inline int StepsForDurationMs(int duration_ms, int tick_ms = kTickMs) {
  if (tick_ms <= 0) {
    tick_ms = kTickMs;
  }
  return duration_ms > 0 ? std::max(1, duration_ms / tick_ms) : 1;
}

inline bool EntryFinished(int step, int steps) { return step >= steps; }

inline bool TimerNeeded(int fadeout_count) { return fadeout_count > 0; }

// Qt FinishPipeline / PipelineFinished: emit Finished only when nothing remains.
inline bool ShouldEmitFinished(bool has_current, int fadeout_count) { return !has_current && fadeout_count <= 0; }

inline bool ShouldEmitFinished(bool has_current, int fadeout_count, int old_count) {
  return !has_current && fadeout_count <= 0 && old_count <= 0;
}

inline bool ShouldDelayExclusivePlay(bool exclusive, int fadeout_count) { return exclusive && fadeout_count > 0; }

// Qt OldExclusivePipelineActive: fadeout or still-tearing-down exclusive pipelines.
inline bool ShouldDelayExclusivePlay(bool exclusive, int fadeout_count, int old_count) {
  return exclusive && (fadeout_count > 0 || old_count > 0);
}

// Qt Load+Play makes the new pipeline current immediately and fades the old one in the map.
inline bool PromoteNextOnPlay(bool has_current, bool has_next) { return has_current && has_next; }

}  // namespace EngineFadeout

#endif
