#include "core/windowgeometry.h"

#include "constants/behavioursettings.h"

#include <algorithm>

namespace WindowGeometry {

State Clamp(State state) {
  state.width = std::clamp(state.width, kMinWidth, kMaxWidth);
  state.height = std::clamp(state.height, kMinHeight, kMaxHeight);
  return state;
}

State FromValues(int width, int height, bool maximized) {
  State state;
  state.width = width > 0 ? width : kDefaultWidth;
  state.height = height > 0 ? height : kDefaultHeight;
  state.maximized = maximized;
  return Clamp(state);
}

int StartupAction(int startup_behaviour, bool remembered_maximized) {
  switch (static_cast<BehaviourSettings::StartupBehaviour>(startup_behaviour)) {
    case BehaviourSettings::StartupBehaviour::Hide:
      return 3;
    case BehaviourSettings::StartupBehaviour::ShowMaximized:
      return 4;
    case BehaviourSettings::StartupBehaviour::ShowMinimized:
      return 5;
    case BehaviourSettings::StartupBehaviour::Show:
      return 2;
    case BehaviourSettings::StartupBehaviour::Remember:
    default:
      return remembered_maximized ? 4 : 2;
  }
}

}  // namespace WindowGeometry
