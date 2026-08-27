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

}  // namespace Windows7ThumbBarActions

#endif
