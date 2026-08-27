#ifndef STRAWBERRY_COMMANDLINEWINDOW_H
#define STRAWBERRY_COMMANDLINEWINDOW_H

#include "core/commandlineoptions.h"

#include <cstdio>
#include <string>

namespace CommandlineWindow {

inline bool ParseSize(const char *value, int *width, int *height) {
  if (!value || !width || !height) {
    return false;
  }
  return std::sscanf(value, "%dx%d", width, height) == 2 && *width > 0 && *height > 0;
}

inline bool ShouldResize(CommandlineOptions::PlayerAction action, int width, int height) {
  return action == CommandlineOptions::PlayerAction::ResizeWindow && width > 0 && height > 0;
}

inline bool ShouldRaise(const CommandlineOptions &options) { return options.is_empty(); }

}  // namespace CommandlineWindow

#endif  // STRAWBERRY_COMMANDLINEWINDOW_H
