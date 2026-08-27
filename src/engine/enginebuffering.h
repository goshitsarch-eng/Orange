#ifndef STRAWBERRY_ENGINEBUFFERING_H
#define STRAWBERRY_ENGINEBUFFERING_H

#include <cstdint>

namespace EngineBuffering {

constexpr int kIgnoreNearEndSeconds = 5;
constexpr int64_t kNsecPerSec = 1000000000LL;
constexpr int kProgressMax = 100;

inline bool IgnoreNearEnd(bool about_to_finish, int64_t position_nanosec, int64_t length_nanosec) {
  return about_to_finish && length_nanosec > 0 && position_nanosec > 0 &&
         (length_nanosec - position_nanosec) < static_cast<int64_t>(kIgnoreNearEndSeconds) * kNsecPerSec;
}

inline bool ShouldStart(int percent, bool already_buffering) { return percent < kProgressMax && !already_buffering; }

inline bool ShouldFinish(int percent, bool already_buffering) { return percent >= kProgressMax && already_buffering; }

// Qt GstEnginePipeline::BufferingMessageReceived pauses PLAYING while filling the queue.
inline bool ShouldPausePlaying(bool already_buffering, int percent, bool is_playing) {
  return ShouldStart(percent, already_buffering) && is_playing;
}

inline bool ShouldRestorePlaying(bool already_buffering, int percent, bool restore_playing) {
  return ShouldFinish(percent, already_buffering) && restore_playing;
}

inline bool ShouldEmitProgress(bool already_buffering, int percent) {
  return already_buffering && percent < kProgressMax;
}

// Qt SourceSetupCallback ends buffering and forces PLAYING after a new HTTP source.
inline bool ShouldClearBufferingOnSourceSetup(bool already_buffering) { return already_buffering; }

inline const char *TaskName() { return "Buffering"; }

}  // namespace EngineBuffering

#endif  // STRAWBERRY_ENGINEBUFFERING_H
