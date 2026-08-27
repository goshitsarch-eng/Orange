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
  if (unique_id.rfind("mtp:", 0) == 0 || unique_id.rfind("MTP/", 0) == 0) {
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

// Qt MtpDevice::ProgressCallback: sent/total as 0..1 during LIBMTP_Send_Track_From_File.
inline float FileFraction(unsigned long long sent, unsigned long long total) {
  return total > 0 ? static_cast<float>(sent) / static_cast<float>(total) : 0.0f;
}

inline float ClampFileFraction(float fraction) {
  if (fraction < 0.0f) {
    return 0.0f;
  }
  if (fraction > 1.0f) {
    return 1.0f;
  }
  return fraction;
}

// Task bar uses hundredths so a long send updates inside the current file.
inline int ScaledProgress(int complete, float current_fraction, int total) {
  if (total <= 0) {
    return 0;
  }
  const float fraction = ClampFileFraction(current_fraction);
  const int value = complete * 100 + static_cast<int>(fraction * 100.0f);
  const int max = total * 100;
  return value > max ? max : value;
}

inline int ScaledProgressMax(int total) { return total > 0 ? total * 100 : 0; }

}  // namespace DeviceCopyJob

#endif  // STRAWBERRY_DEVICECOPYJOB_H
