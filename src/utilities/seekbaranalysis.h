#ifndef STRAWBERRY_SEEKBARANALYSIS_H
#define STRAWBERRY_SEEKBARANALYSIS_H

#include "core/song.h"
#include "moodbar/moodbarcell.h"
#include "utilities/analysisasync.h"

#include <string>

namespace SeekbarAnalysis {

inline bool CanLoad(const Song &song) { return MoodbarCell::CanLoad(song); }

inline bool ShouldGenerate(bool enabled, const Song &song) { return enabled && !song.url().empty(); }

inline bool ShouldGenerateOnEnable(bool enabled, bool was_enabled, const std::string &url) {
  return enabled && !was_enabled && !url.empty();
}

inline bool ShouldClearOnDisable(bool enabled, bool was_enabled) { return !enabled && was_enabled; }

inline bool ShouldClearOnStop(bool enabled) { return enabled; }

inline bool AcceptResult(bool enabled, bool playback_active, const std::string &job_url, const std::string &current_url, int job_generation,
                         int current_generation, bool alive) {
  return enabled && playback_active && job_url == current_url && AnalysisAsync::AcceptGeneration(job_generation, current_generation, alive);
}

}  // namespace SeekbarAnalysis

#endif  // STRAWBERRY_SEEKBARANALYSIS_H
