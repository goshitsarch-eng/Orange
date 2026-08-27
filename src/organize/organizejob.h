#ifndef STRAWBERRY_ORGANIZEJOB_H
#define STRAWBERRY_ORGANIZEJOB_H

#include "core/song.h"

namespace OrganizeJob {

inline constexpr int kBatchSize = 10;

inline const char *TaskName() { return "Organizing files"; }

inline bool ShouldProcessBatch(bool cancelled, bool waiting_for_transcode) { return !cancelled && !waiting_for_transcode; }

inline bool ShouldWaitForTranscode(int pending) { return pending > 0; }

inline bool ShouldFinish(int next, int total, bool waiting_for_transcode, bool cancelled) {
  if (cancelled) {
    return !waiting_for_transcode;
  }
  return next >= total && !waiting_for_transcode;
}

inline bool ShouldScheduleNext(int next, int total, bool waiting_for_transcode, bool cancelled, bool async) {
  return async && !cancelled && !waiting_for_transcode && next < total;
}

inline int Progress(int complete) { return complete * 100; }

inline int ProgressMax(int total) { return total * 100; }

inline bool ShouldSkipInvalid(const Song &song) { return !song.is_valid(); }

inline bool ShouldSkipExisting(bool overwrite, bool dest_exists) { return !overwrite && dest_exists; }

}  // namespace OrganizeJob

#endif  // STRAWBERRY_ORGANIZEJOB_H
