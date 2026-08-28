#ifndef STRAWBERRY_ENGINEEOS_H
#define STRAWBERRY_ENGINEEOS_H

#include <string>

// What the engine does when a pipeline reports end-of-stream, and what the Load() that follows a track change
// is allowed to do with the pipelines that are already around.
namespace EngineEos {

enum class Action {
  Ignore,
  RemoveFadeout,
  // playbin was given a next-uri and has already moved onto it by itself.
  ContinueGapless,
  EndTrack,
};

inline Action ActionFor(bool is_fadeout_pipeline, bool is_current_pipeline, bool gapless_pending) {
  if (is_fadeout_pipeline) {
    return Action::RemoveFadeout;
  }
  if (!is_current_pipeline) {
    return Action::Ignore;
  }
  return gapless_pending ? Action::ContinueGapless : Action::EndTrack;
}

// Both ways of reaching the next track have to tell the player the track ended, otherwise the playlist never
// advances and nothing is counted as played or scrobbled.  A preloaded second pipeline is built but never
// started, so it is not a crossfade in flight and must not be promoted here.
inline bool EmitsTrackEnded(Action action) { return action == Action::ContinueGapless || action == Action::EndTrack; }

// A pipeline that has reached the end of its stream will never play a next-uri, so it cannot be continued.
inline bool CanContinueIntoNextUri(bool auto_change, bool has_valid_current, bool current_ended, bool crossfade) {
  return auto_change && has_valid_current && !current_ended && !crossfade;
}

// Adopting the pipeline StartPreloading already built and prerolled is what makes the track change gapless.
inline bool ShouldAdoptPreloaded(bool has_valid_next, const std::string &next_url, const std::string &url) {
  return has_valid_next && !url.empty() && next_url == url;
}

// After a gapless continuation the running pipeline is already playing the URL the player is asking for.
inline bool AlreadyPlaying(bool gapless_continued, bool auto_change, bool has_valid_current, const std::string &stream_url,
                           const std::string &url) {
  return gapless_continued && auto_change && has_valid_current && !url.empty() && stream_url == url;
}

}  // namespace EngineEos

#endif
