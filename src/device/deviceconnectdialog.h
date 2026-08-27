#ifndef STRAWBERRY_DEVICECONNECTDIALOG_H
#define STRAWBERRY_DEVICECONNECTDIALOG_H

#include "device/connecteddevice.h"
#include "device/deviceviewlook.h"

#include <string>

namespace DeviceConnectDialog {

inline const char *Title() { return "Connect device"; }

inline const char *Message() {
  return "This is the first time you have connected this device.  Strawberry will now scan the device to find music files - this may take some time.";
}

inline const char *Accept() { return "Connect device"; }

inline const char *Cancel() { return "Cancel"; }

inline bool AskForScan(const std::string &backend) { return backend != "cdda"; }

inline bool NeedsMount(const ConnectedDevice &device) { return DeviceViewLook::ShouldMountOnActivate(device); }

inline bool NeedsFirstConnectPrompt(bool stored_in_db, bool needs_mount, const std::string &backend) {
  return !stored_in_db && !needs_mount && AskForScan(backend);
}

}  // namespace DeviceConnectDialog

#endif  // STRAWBERRY_DEVICECONNECTDIALOG_H
