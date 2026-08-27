#ifndef STRAWBERRY_ENGINEFADE_H
#define STRAWBERRY_ENGINEFADE_H

#include <algorithm>

namespace EngineFade {

// Qt GstEngine::Stop: fadeout_enabled_ && !stop_after && !AnyExclusivePipelineActive().
// Skip when the pipeline is already idle (post-EOS cleanup).
inline bool ShouldFadeOnStop(bool fadeout_enabled, bool stop_after, bool exclusive_active, bool already_idle) {
  return fadeout_enabled && !stop_after && !exclusive_active && !already_idle;
}

// Qt GstEngine::Pause / Unpause: skip fade when exclusive output is active.
inline bool ShouldFadeOnPause(bool fadeout_pause_enabled, bool exclusive_active) {
  return fadeout_pause_enabled && !exclusive_active;
}

// Linear volume at fade progress t ∈ [0, 1]. direction < 0 fades out.
inline double VolumeAtStep(double target_fraction, int direction, double t) {
  t = std::clamp(t, 0.0, 1.0);
  if (direction < 0) {
    return target_fraction * (1.0 - t);
  }
  return target_fraction * t;
}

}  // namespace EngineFade

#endif  // STRAWBERRY_ENGINEFADE_H
