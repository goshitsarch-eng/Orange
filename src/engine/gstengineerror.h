#ifndef STRAWBERRY_GSTENGINEERROR_H
#define STRAWBERRY_GSTENGINEERROR_H

#include <gst/gst.h>

namespace GstEngineError {

enum class Kind { Fatal, InvalidSong };

// Qt GstEngine::HandlePipelineError: missing/unreadable/unauthorized resources
// and any stream error skip the song. Device/sink/core failures stop playback.
inline bool IsInvalidSongResourceCode(int error_code) {
  return error_code == static_cast<int>(GST_RESOURCE_ERROR_NOT_FOUND) ||
         error_code == static_cast<int>(GST_RESOURCE_ERROR_OPEN_READ) ||
         error_code == static_cast<int>(GST_RESOURCE_ERROR_NOT_AUTHORIZED);
}

inline bool IsInvalidSongError(int domain, int error_code) {
  if (domain == static_cast<int>(GST_RESOURCE_ERROR) && IsInvalidSongResourceCode(error_code)) {
    return true;
  }
  return domain == static_cast<int>(GST_STREAM_ERROR);
}

inline Kind Classify(int domain, int error_code) {
  return IsInvalidSongError(domain, error_code) ? Kind::InvalidSong : Kind::Fatal;
}

inline bool ShouldTearDownCurrent(bool has_current, int current_id, int pipeline_id) {
  return has_current && current_id == pipeline_id;
}

inline bool ShouldStopOnInvalidSong(bool continue_on_error) { return !continue_on_error; }

}  // namespace GstEngineError

#endif  // STRAWBERRY_GSTENGINEERROR_H
