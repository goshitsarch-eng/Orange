#ifndef STRAWBERRY_ENGINEPLAY_H
#define STRAWBERRY_ENGINEPLAY_H

#include <cstdint>

namespace EnginePlay {

// Qt GstEngine::Play returns false while the current pipeline is buffering.
inline bool IsBuffering(int buffering_task_id) { return buffering_task_id != -1; }

// Qt GstEngine::Play: when already PLAYING, Seek + PlayDone only if a non-zero offset is requested or the track has a beginning offset.
inline bool ShouldSeekWhenAlreadyPlaying(uint64_t offset_nanosec, uint64_t beginning_offset_nanosec) {
  return offset_nanosec != 0 || beginning_offset_nanosec != 0;
}

// Qt GstEngine::Play short-circuits before calling GstEnginePipeline::Play when the pipeline is already PLAYING.
inline bool ShouldShortCircuitPlayingPipeline(bool pipeline_playing, int buffering_task_id) {
  return pipeline_playing && !IsBuffering(buffering_task_id);
}

}  // namespace EnginePlay

#endif  // STRAWBERRY_ENGINEPLAY_H
