#ifndef STRAWBERRY_DEVICECOPYJOB_H
#define STRAWBERRY_DEVICECOPYJOB_H

#include "device/connecteddevice.h"

#include <string>

namespace DeviceCopyJob {

inline constexpr int kBatchSize = 10;

inline const char *TaskName() { return "Copying to device"; }

// Qt Organize always goes through MusicStorage (MTP/iPod included). GTK still copies those via DeviceCopyRunner.
inline bool UsesDeviceCopyRunner(const std::string &backend) { return backend == "gpod" || backend == "mtp"; }

inline bool UsesDeviceCopyRunner(const ConnectedDevice &device) { return UsesDeviceCopyRunner(device.backend); }

// Qt OrganizeDialog is used for every connected device, including MTP and iPod.
inline bool ShouldUseOrganizeDialog(const ConnectedDevice &device) {
  return UsesDeviceCopyRunner(device) || !device.mount_path.empty();
}

inline std::string MtpSerial(const std::string &unique_id) {
  if (unique_id.rfind("mtp:", 0) == 0) {
    return unique_id.substr(4);
  }
  return unique_id;
}

inline bool ShouldProcessBatch(bool cancelled) { return !cancelled; }

inline bool ShouldFinish(int next, int total, bool cancelled) { return cancelled || next >= total; }

inline bool ShouldScheduleNext(int next, int total, bool cancelled, bool async) {
  return async && !cancelled && next < total;
}

inline int Progress(int complete) { return complete; }

inline int ProgressMax(int total) { return total; }

}  // namespace DeviceCopyJob

#endif  // STRAWBERRY_DEVICECOPYJOB_H
