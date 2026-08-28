#ifndef STRAWBERRY_WINDOWS7THUMBBARACTIONS_H
#define STRAWBERRY_WINDOWS7THUMBBARACTIONS_H

#include <string>
#include <vector>

namespace Windows7ThumbBarActions {

inline constexpr int kIconSize = 16;
inline constexpr int kMaxButtonCount = 7;
inline constexpr int kUpdateDelayMs = 300;

enum class Id { Previous, PlayPause, Stop, Next, Spacer, Love };

enum class Flag { Enabled, Disabled, Hidden, NoBackground };

inline std::vector<Id> DefaultActions() {
  return {Id::Previous, Id::PlayPause, Id::Stop, Id::Next, Id::Spacer, Id::Love};
}

inline bool IsSpacer(Id id) { return id == Id::Spacer; }

inline const char *IconName(Id id, bool playing) {
  switch (id) {
    case Id::Previous:
      return "media-skip-backward-symbolic";
    case Id::PlayPause:
      return playing ? "media-playback-pause-symbolic" : "media-playback-start-symbolic";
    case Id::Stop:
      return "media-playback-stop-symbolic";
    case Id::Next:
      return "media-skip-forward-symbolic";
    case Id::Love:
      return "emblem-favorite-symbolic";
    case Id::Spacer:
      return "";
  }
  return "";
}

inline Flag FlagFor(bool spacer, bool visible, bool enabled) {
  if (spacer) {
    return Flag::NoBackground;
  }
  if (!visible) {
    return Flag::Hidden;
  }
  return enabled ? Flag::Enabled : Flag::Disabled;
}

inline bool WithinLimit(int count) { return count <= kMaxButtonCount; }

inline const char *Tooltip(Id id) {
  switch (id) {
    case Id::Previous:
      return "Previous";
    case Id::PlayPause:
      return "Play/Pause";
    case Id::Stop:
      return "Stop";
    case Id::Next:
      return "Next";
    case Id::Love:
      return "Love";
    case Id::Spacer:
      return "";
  }
  return "";
}

inline const char *PlayerAction(Id id) {
  switch (id) {
    case Id::Previous:
      return "prev_track";
    case Id::PlayPause:
      return "play_pause";
    case Id::Stop:
      return "stop";
    case Id::Next:
      return "next_track";
    case Id::Love:
      return "love";
    case Id::Spacer:
      return "";
  }
  return "";
}

inline bool ShouldDispatch(Id id) { return !IsSpacer(id); }

inline bool ShouldRebuildOnPlayingChange(bool was_playing, bool playing) { return was_playing != playing; }

inline bool ShouldAddButtons(bool taskbar_created, bool already_added) { return taskbar_created && !already_added; }

inline bool ShouldUpdateButtons(bool already_added) { return already_added; }

inline int ClampTipChars(int length) {
  if (length < 0) {
    return 0;
  }
  return length > 259 ? 259 : length;
}

// commctrl THBF_* / THB_* so tests do not need windows.h.
inline constexpr unsigned kThbfEnabled = 0;
inline constexpr unsigned kThbfDisabled = 0x00000001;
inline constexpr unsigned kThbfNoBackground = 0x00000004;
inline constexpr unsigned kThbfHidden = 0x00000008;
inline constexpr unsigned kThbIcon = 0x00000001;
inline constexpr unsigned kThbTooltip = 0x00000002;
inline constexpr unsigned kThbFlags = 0x00000004;
inline constexpr unsigned kWmCommand = 0x0111;

inline unsigned WinFlags(Flag flag) {
  switch (flag) {
    case Flag::Disabled:
      return kThbfDisabled;
    case Flag::Hidden:
      return kThbfHidden;
    case Flag::NoBackground:
      return kThbfNoBackground;
    case Flag::Enabled:
    default:
      return kThbfEnabled;
  }
}

inline unsigned ButtonMask(bool spacer) { return spacer ? kThbFlags : (kThbIcon | kThbTooltip | kThbFlags); }

inline Id ActionAtCommand(int cmd, const std::vector<Id> &actions) {
  if (cmd < 0 || static_cast<size_t>(cmd) >= actions.size()) {
    return Id::Spacer;
  }
  return actions[static_cast<size_t>(cmd)];
}

inline int CommandId(unsigned wparam) { return static_cast<int>(wparam & 0xFFFFu); }

enum class WinMessage { TaskbarCreated, Command, Other };

inline WinMessage ClassifyMessage(unsigned msg, unsigned taskbar_created_id) {
  if (taskbar_created_id != 0 && msg == taskbar_created_id) {
    return WinMessage::TaskbarCreated;
  }
  if (msg == kWmCommand) {
    return WinMessage::Command;
  }
  return WinMessage::Other;
}

}  // namespace Windows7ThumbBarActions

#endif
