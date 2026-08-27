#ifndef STRAWBERRY_ENGINEEXCLUSIVE_H
#define STRAWBERRY_ENGINEEXCLUSIVE_H

namespace EngineExclusive {

// Qt never runs two exclusive sinks (crossfade / preload next pipeline).
inline bool AllowsSecondPipeline(bool exclusive) { return !exclusive; }

inline bool ShouldCrossfade(bool want_crossfade, bool exclusive) { return want_crossfade && !exclusive; }

// Qt GstEngine::Play: OldExclusivePipelineActive() delays Play until fadeout finishes.
inline bool ShouldDelayPlay(bool exclusive, bool exclusive_fadeout_active) { return exclusive && exclusive_fadeout_active; }

}  // namespace EngineExclusive

#endif  // STRAWBERRY_ENGINEEXCLUSIVE_H
