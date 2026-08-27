#ifndef STRAWBERRY_ORGANIZESTORAGE_H
#define STRAWBERRY_ORGANIZESTORAGE_H

#include "core/musicstorage.h"
#include "device/devicestorage.h"

#include <string>

namespace OrganizeStorage {

// Qt Organize allows an empty destination string when MusicStorage::LocalPath() is empty (MTP).
inline bool AllowsEmptyDestination(const std::string &local_path) { return local_path.empty(); }

inline bool AllowsEmptyDestination(const MusicStorage *storage) { return storage && AllowsEmptyDestination(storage->LocalPath()); }

inline std::string CopyDestination(const std::string &local_path, const std::string &relative) {
  return DeviceStorage::CopyDestination(local_path, relative);
}

inline bool DestinationExists(const std::string &local_path, const std::string &relative) {
  return DeviceStorage::DestinationExists(local_path, relative);
}

inline bool ShouldMkdir(const std::string &local_path) { return DeviceStorage::ShouldMkdir(local_path); }

inline std::string LocalPathOr(const MusicStorage *storage, const std::string &destination) {
  if (storage && !storage->LocalPath().empty()) {
    return storage->LocalPath();
  }
  return destination;
}

}  // namespace OrganizeStorage

#endif
