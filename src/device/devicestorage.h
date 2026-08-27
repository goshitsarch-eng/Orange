#ifndef STRAWBERRY_DEVICESTORAGE_H
#define STRAWBERRY_DEVICESTORAGE_H

#include "device/connecteddevice.h"
#include "utilities/fileutils.h"

#include <string>

namespace DeviceStorage {

enum class Kind { None, Filesystem, Mtp, GPod };

// Qt DeviceManager Role_Storage: MTP and iPod are MusicStorage, not a separate copy runner.
inline Kind KindFor(const std::string &backend, const std::string &mount_path) {
  if (backend == "mtp") {
    return Kind::Mtp;
  }
  if (backend == "gpod") {
    return Kind::GPod;
  }
  if (!mount_path.empty()) {
    return Kind::Filesystem;
  }
  return Kind::None;
}

inline Kind KindFor(const ConnectedDevice &device) { return KindFor(device.backend, device.mount_path); }

inline bool UsesOrganizeMusicStorage(Kind kind) { return kind == Kind::Mtp || kind == Kind::GPod; }

inline bool UsesOrganizeMusicStorage(const std::string &backend) { return UsesOrganizeMusicStorage(KindFor(backend, {})); }

inline bool UsesOrganizeMusicStorage(const ConnectedDevice &device) { return UsesOrganizeMusicStorage(KindFor(device)); }

inline bool AllowsEmptyDestination(Kind kind) { return kind == Kind::Mtp; }

inline std::string CopyDestination(const std::string &local_path, const std::string &relative) {
  if (local_path.empty()) {
    return relative;
  }
  return FileUtils::Join(local_path, relative);
}

inline bool DestinationExists(const std::string &local_path, const std::string &relative) {
  if (local_path.empty() || relative.empty()) {
    return false;
  }
  return FileUtils::Exists(FileUtils::Join(local_path, relative));
}

inline bool ShouldMkdir(const std::string &local_path) { return !local_path.empty(); }

inline bool ShouldStartCopy(Kind kind) { return kind != Kind::None; }

inline bool ShouldFinishCopy(Kind kind) { return kind != Kind::None; }

}  // namespace DeviceStorage

#endif
