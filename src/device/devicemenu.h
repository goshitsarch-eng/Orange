#ifndef STRAWBERRY_DEVICEMENU_H
#define STRAWBERRY_DEVICEMENU_H

#include "device/connecteddevice.h"
#include "device/devicecopy.h"

#include <cstring>
#include <vector>

namespace DeviceMenu {

enum class Action { Browse, CopyPlaylist, Properties, Unmount, Forget };

struct Item {
  const char *label = "";
  const char *id = "";
  Action action = Action::Browse;
};

struct DeviceState {
  bool connected = false;
  bool remembered = false;
  bool mountable = false;
  bool filesystem = false;
};

inline std::vector<Item> Items() {
  return {
      {"Browse", "browse", Action::Browse},
      {"Copy playlist…", "copy", Action::CopyPlaylist},
      {"Device properties...", "properties", Action::Properties},
      {"Safely remove device", "unmount", Action::Unmount},
      {"Forget device", "forget", Action::Forget},
  };
}

inline int ItemCount() { return static_cast<int>(Items().size()); }

inline Action FromId(const char *id) {
  if (!id) {
    return Action::Browse;
  }
  for (const Item &item : Items()) {
    if (std::strcmp(item.id, id) == 0) {
      return item.action;
    }
  }
  return Action::Browse;
}

inline DeviceState FromDevice(const ConnectedDevice &device, bool remembered) {
  DeviceState state;
  state.connected = !device.unique_id.empty();
  state.remembered = remembered;
  state.mountable = !device.mount_path.empty();
  state.filesystem = DeviceCopy::IsFilesystemDevice(device);
  return state;
}

inline bool ItemEnabled(Action action, const DeviceState &state) {
  switch (action) {
    case Action::Browse:
    case Action::CopyPlaylist:
    case Action::Unmount:
      return state.connected;
    case Action::Properties:
      return true;
    case Action::Forget:
      return state.remembered;
  }
  return false;
}

inline bool UnmountEnabled(const DeviceState &state) { return ItemEnabled(Action::Unmount, state); }

inline bool ForgetEnabled(const DeviceState &state) { return ItemEnabled(Action::Forget, state); }

inline bool IncludeItem(const Item &item, const DeviceState &state) { return ItemEnabled(item.action, state); }

inline std::vector<Item> VisibleItems(const DeviceState &state) {
  std::vector<Item> visible;
  for (const Item &item : Items()) {
    if (IncludeItem(item, state)) {
      visible.push_back(item);
    }
  }
  return visible;
}

inline bool Contains(const std::vector<Item> &items, Action action) {
  for (const Item &item : items) {
    if (item.action == action) {
      return true;
    }
  }
  return false;
}

}  // namespace DeviceMenu

#endif
