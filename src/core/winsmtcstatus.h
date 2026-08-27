#ifndef STRAWBERRY_WINSMTCSTATUS_H
#define STRAWBERRY_WINSMTCSTATUS_H

#include "engine/enginebase.h"

#include <cstdint>
#include <string>
#include <vector>

namespace WinSmtcStatus {

enum class Playback { Closed, Changing, Stopped, Playing, Paused };

// ABI::Windows::Media::SystemMediaTransportControlsButton values.
enum class Button {
  Play = 0,
  Pause = 1,
  Stop = 2,
  Record = 3,
  Next = 4,
  Previous = 5,
  FastForward = 6,
  Rewind = 7,
  ChannelUp = 8,
  ChannelDown = 9
};

// Qt maps Error (and every non-playing/paused state) to Stopped, not Closed.
inline Playback FromEngine(EngineBase::State state) {
  switch (state) {
    case EngineBase::State::Playing:
      return Playback::Playing;
    case EngineBase::State::Paused:
      return Playback::Paused;
    case EngineBase::State::Error:
    case EngineBase::State::Empty:
    case EngineBase::State::Idle:
    default:
      return Playback::Stopped;
  }
}

inline bool ButtonsEnabled(Playback playback) { return playback == Playback::Playing || playback == Playback::Paused; }

inline bool TimelineEnabled(Playback playback) { return playback != Playback::Closed && playback != Playback::Stopped; }

inline bool ShouldRunTimelineTimer(EngineBase::State state) { return state == EngineBase::State::Playing; }

inline bool ShouldClearMetadata(bool song_valid) { return !song_valid; }

inline bool ShouldApplyCover(const std::string &song_url, const std::string &current_url) { return song_url == current_url; }

inline bool ShouldSetThumbnail(bool url_matches, bool has_image) { return url_matches && has_image; }

inline bool ShouldClearThumbnail(bool url_matches, bool has_image) { return url_matches && !has_image; }

inline bool LooksLikeJpeg(const std::vector<unsigned char> &data) {
  return data.size() >= 3 && data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF;
}

inline int64_t TimelineHundredNs(int64_t nanosec) { return nanosec / 100; }

inline int64_t TimelineDurationNs(int64_t song_length_ns, int64_t engine_length_ns) {
  return song_length_ns > 0 ? song_length_ns : engine_length_ns;
}

inline const char *ButtonAction(Button button) {
  switch (button) {
    case Button::Play:
      return "play";
    case Button::Pause:
      return "pause";
    case Button::Stop:
      return "stop";
    case Button::Next:
      return "next";
    case Button::Previous:
      return "previous";
    default:
      return "";
  }
}

inline Button ButtonFromWinRt(int button) { return static_cast<Button>(button); }

inline bool DispatchesToPlayer(Button button) {
  switch (button) {
    case Button::Play:
    case Button::Pause:
    case Button::Stop:
    case Button::Next:
    case Button::Previous:
      return true;
    default:
      return false;
  }
}

}  // namespace WinSmtcStatus

#endif
