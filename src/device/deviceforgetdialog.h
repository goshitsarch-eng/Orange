#ifndef STRAWBERRY_DEVICEFORGETDIALOG_H
#define STRAWBERRY_DEVICEFORGETDIALOG_H

#include "device/deviceconnectdialog.h"

#include <string>

namespace DeviceForgetDialog {

inline const char *Title() { return "Forget device"; }

inline const char *Message() {
  return "Forgetting a device will remove it from this list and Strawberry will have to rescan all the songs again next time you connect it.";
}

inline const char *Accept() { return "Forget device"; }

inline const char *Cancel() { return "Cancel"; }

inline bool NeedsPrompt(const std::string &backend) { return DeviceConnectDialog::AskForScan(backend); }

}  // namespace DeviceForgetDialog

#endif  // STRAWBERRY_DEVICEFORGETDIALOG_H
