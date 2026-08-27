#ifndef STRAWBERRY_DEVICEPROPERTIESINFO_H
#define STRAWBERRY_DEVICEPROPERTIESINFO_H

#include "device/connecteddevice.h"
#include "utilities/fileutils.h"

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

namespace DevicePropertiesInfo {

struct Row {
  std::string key;
  std::string value;
};

struct Space {
  bool available = false;
  int64_t total = 0;
  int64_t free = 0;
};

inline void AppendRow(std::vector<Row> *rows, const char *key, const std::string &value) {
  if (!rows || !key || value.empty()) {
    return;
  }
  rows->push_back({key, value});
}

inline std::vector<Row> Rows(const ConnectedDevice &device) {
  std::vector<Row> rows;
  AppendRow(&rows, "Backend", device.backend);
  AppendRow(&rows, "Unique ID", device.unique_id);
  AppendRow(&rows, "Mount", device.mount_path);
  if (device.size > 0) {
    AppendRow(&rows, "Capacity", FileUtils::PrettySize(device.size));
  }
  AppendRow(&rows, "Icon", device.icon);
  std::sort(rows.begin(), rows.end(), [](const Row &a, const Row &b) { return a.key < b.key; });
  return rows;
}

inline bool HasHardwareInfo(const ConnectedDevice &device) {
  return !device.unique_id.empty() || !device.mount_path.empty() || !device.backend.empty();
}

inline Space SpaceFor(const ConnectedDevice &device) {
  Space space;
  if (device.mount_path.empty()) {
    return space;
  }
  space.free = FileUtils::FreeSpaceBytes(device.mount_path);
  space.total = device.size > 0 ? device.size : FileUtils::TotalSpaceBytes(device.mount_path);
  space.available = space.free >= 0 && space.total > 0;
  return space;
}

inline bool OpenEnabled(const ConnectedDevice &device) { return !device.mount_path.empty(); }

}  // namespace DevicePropertiesInfo

#endif
