#ifndef STRAWBERRY_DEVICEEJECT_H
#define STRAWBERRY_DEVICEEJECT_H

#include "device/connecteddevice.h"
#include "device/devicemenu.h"

#include <string>

namespace DeviceEject {

// Qt ConnectedDevice::Eject only works when the lister can unmount a mount path.
inline bool Supported(const std::string &mount_path) { return !mount_path.empty(); }

inline bool Supported(const ConnectedDevice &device) { return Supported(device.mount_path); }

inline bool ShouldShowCheckbox(bool show_eject_request, bool supported) { return show_eject_request && supported; }

inline bool ShouldEjectAfter(bool checkbox_visible, bool checked, bool copy_succeeded) {
  return checkbox_visible && checked && copy_succeeded;
}

inline bool MatchesUnmountMenu(const DeviceMenu::DeviceState &state) { return DeviceMenu::UnmountEnabled(state); }

}  // namespace DeviceEject

#endif
