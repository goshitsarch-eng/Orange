#ifndef STRAWBERRY_DEVICEEJECT_H
#define STRAWBERRY_DEVICEEJECT_H

#include "device/connecteddevice.h"
#include "device/devicemenu.h"

#include <functional>
#include <string>

namespace DeviceEject {

enum class UnmountAction { None, VolumeEject, MountEject, MountUnmount, FileUnmount };

// Qt ConnectedDevice::Eject only works when the lister can unmount a mount path.
inline bool Supported(const std::string &mount_path) { return !mount_path.empty(); }

inline bool Supported(const ConnectedDevice &device) { return Supported(device.mount_path); }

inline bool ShouldShowCheckbox(bool show_eject_request, bool supported) { return show_eject_request && supported; }

inline bool ShouldEjectAfter(bool checkbox_visible, bool checked, bool copy_succeeded) {
  return checkbox_visible && checked && copy_succeeded;
}

inline bool MatchesUnmountMenu(const DeviceMenu::DeviceState &state) { return DeviceMenu::UnmountEnabled(state); }

// Qt GioLister::UnmountDevice returns immediately for mtp:// roots.
inline bool SkipsUnmount(const std::string &uri_or_path) { return uri_or_path.rfind("mtp://", 0) == 0; }

inline std::string NormalizeMountPath(std::string path) {
  while (path.size() > 1 && path.back() == '/') {
    path.pop_back();
  }
  return path;
}

inline bool SameMountPath(const std::string &left, const std::string &right) {
  return NormalizeMountPath(left) == NormalizeMountPath(right);
}

// Qt prefers g_volume_eject, then g_mount_eject, then g_mount_unmount.
// FileUnmount keeps the previous GTK path when GIO has no volume.
inline UnmountAction ActionFor(bool skip_mtp, bool has_volume, bool volume_can_eject, bool has_mount, bool mount_can_eject,
                               bool mount_can_unmount, bool has_path) {
  if (skip_mtp) {
    return UnmountAction::None;
  }
  if (has_volume && volume_can_eject) {
    return UnmountAction::VolumeEject;
  }
  if (has_volume && has_mount && mount_can_eject) {
    return UnmountAction::MountEject;
  }
  if (has_volume && has_mount && mount_can_unmount) {
    return UnmountAction::MountUnmount;
  }
  if (has_path) {
    return UnmountAction::FileUnmount;
  }
  return UnmountAction::None;
}

inline bool ShouldAttachEjectHandler(const ConnectedDevice &device) { return Supported(device); }

// Qt OrganizeDialog does not unmount; ConnectedDevice::Eject runs from Organize::Complete.
inline bool DialogUnmountsAfterOrganize() { return false; }

bool UnmountPath(const std::string &mount_path, std::function<void()> finished = {});

}  // namespace DeviceEject

#endif
