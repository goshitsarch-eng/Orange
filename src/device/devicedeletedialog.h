#ifndef STRAWBERRY_DEVICEDELETEDIALOG_H
#define STRAWBERRY_DEVICEDELETEDIALOG_H

namespace DeviceDeleteDialog {

inline const char *Title() { return "Delete files"; }

inline const char *Message() { return "These files will be deleted from the device, are you sure you want to continue?"; }

inline const char *Accept() { return "Yes"; }

inline const char *Cancel() { return "Cancel"; }

}  // namespace DeviceDeleteDialog

#endif  // STRAWBERRY_DEVICEDELETEDIALOG_H
