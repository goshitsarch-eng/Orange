#ifndef STRAWBERRY_SEEKBARFADE_H
#define STRAWBERRY_SEEKBARFADE_H

#include "waveform/waveformplayhead.h"

#include <algorithm>

namespace SeekbarFade {

// Qt MoodbarProxyStyle / WaveformProxyStyle fade state machine.
enum class State {
  Off,
  On,
  FadingToOn,
  FadingToOff,
};

inline constexpr int kDurationMs = WaveformPlayhead::kFadeDurationMs;
inline constexpr int kTickMs = 16;

inline bool AlreadyHeading(State state, bool visible) {
  return (visible && (state == State::On || state == State::FadingToOn)) ||
         (!visible && (state == State::Off || state == State::FadingToOff));
}

inline bool StartsFreshFade(State state) { return state == State::On || state == State::Off; }

inline bool IsFading(State state) { return state == State::FadingToOn || state == State::FadingToOff; }

inline bool WidgetVisible(State state) { return state != State::Off; }

inline int ReverseElapsed(int elapsed_ms) { return std::clamp(kDurationMs - std::max(elapsed_ms, 0), 0, kDurationMs); }

inline float FadeValue(State state, int elapsed_ms) {
  const float t = std::clamp(static_cast<float>(elapsed_ms) / static_cast<float>(kDurationMs), 0.0F, 1.0F);
  switch (state) {
    case State::On:
      return 1.0F;
    case State::FadingToOn:
      return t;
    case State::FadingToOff:
      return 1.0F - t;
    case State::Off:
      break;
  }
  return 0.0F;
}

inline State FinishIfDone(State state, int elapsed_ms) {
  if (elapsed_ms >= kDurationMs) {
    if (state == State::FadingToOn) {
      return State::On;
    }
    if (state == State::FadingToOff) {
      return State::Off;
    }
  }
  return state;
}

struct Machine {
  State state = State::Off;
  int elapsed_ms = 0;

  void Snap(bool visible) {
    state = visible ? State::On : State::Off;
    elapsed_ms = 0;
  }

  void Request(bool visible) {
    if (AlreadyHeading(state, visible)) {
      return;
    }
    if (!StartsFreshFade(state)) {
      elapsed_ms = ReverseElapsed(elapsed_ms);
    } else {
      elapsed_ms = 0;
    }
    state = visible ? State::FadingToOn : State::FadingToOff;
  }

  bool Tick(int dt_ms) {
    if (!IsFading(state)) {
      return false;
    }
    elapsed_ms += dt_ms;
    state = FinishIfDone(state, elapsed_ms);
    if (!IsFading(state)) {
      elapsed_ms = 0;
      return false;
    }
    return true;
  }

  float Opacity() const { return FadeValue(state, elapsed_ms); }

  bool Visible() const { return WidgetVisible(state); }
};

// Qt draws the normal slider unless a fancy proxy is fully on.
inline bool ShowSlider(const Machine &moodbar, const Machine &waveform) {
  return moodbar.state != State::On && waveform.state != State::On;
}

}  // namespace SeekbarFade

#endif
