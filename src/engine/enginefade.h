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

// Qt GstEngine::Unpause: has_faded_out_to_pause_ forces fade-in even when FadeoutPauseEnabled is off.
inline bool ShouldFadeInOnResume(bool faded_out_to_pause, bool fadeout_pause_enabled, bool exclusive_active) {
  return !exclusive_active && (faded_out_to_pause || fadeout_pause_enabled);
}

// Qt FadeoutPauseFinished + StopFadeoutPause: latch after pause-fade completes or is aborted.
inline bool ShouldMarkFadedOutToPause(bool pause_fade_active) { return pause_fade_active; }

// Qt GstEngine::FinishPipeline: emit Finished when no current or fadeout pipeline remains.
inline bool ShouldEmitFinished(bool has_current, bool has_fadeout) { return !has_current && !has_fadeout; }

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
