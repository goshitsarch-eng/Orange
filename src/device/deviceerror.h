#ifndef STRAWBERRY_DEVICEERROR_H
#define STRAWBERRY_DEVICEERROR_H

#include <string>

namespace DeviceError {

inline const char *MissingDevice() { return "Device not found"; }

inline const char *CopyFailed() { return "Could not copy songs to the device"; }

inline const char *DeleteFailed() { return "Could not delete the song from the device"; }

inline const char *MountFailed() { return "Could not mount the device"; }

inline const char *UnmountFailed() { return "Could not unmount the device"; }

inline const char *MtpCopyFailed() { return "Could not copy the MTP track"; }

inline std::string ForCopy(bool device_found, bool copied) {
  if (!device_found) {
    return MissingDevice();
  }
  if (!copied) {
    return CopyFailed();
  }
  return {};
}

}  // namespace DeviceError

#endif
