#ifndef STRAWBERRY_DEVICEDELETEJOB_H
#define STRAWBERRY_DEVICEDELETEJOB_H

#include "core/song.h"
#include "device/connecteddevice.h"
#include "device/devicestorage.h"

#include <algorithm>
#include <set>
#include <string>

namespace DeviceDeleteJob {

// Qt DeviceView::Delete uses DeleteFiles with use_trash = false.
inline bool UseTrash() { return false; }

inline bool ShouldUseDeleteFiles(const std::string &backend, const std::string &mount_path) {
  if (backend == "cdda") {
    return false;
  }
  return DeviceStorage::KindFor(backend, mount_path) != DeviceStorage::Kind::None;
}

inline bool ShouldUseDeleteFiles(const ConnectedDevice &device) { return ShouldUseDeleteFiles(device.backend, device.mount_path); }

// Qt DeviceView::DeleteFinished shows OrganizeErrorDialog only when some files failed.
inline bool ShouldShowErrorDialog(const SongList &errors) { return !errors.empty(); }

inline bool ShouldRefreshAfterDelete(const std::string &backend, int deleted) { return deleted > 0 && backend != "cdda"; }

inline int SongCountAfterDelete(int previous, int deleted) {
  if (previous < 0) {
    return previous;
  }
  return std::max(0, previous - deleted);
}

inline SongList Succeeded(const SongList &requested, const SongList &errors) {
  std::set<std::string> failed;
  for (const Song &song : errors) {
    failed.insert(song.url());
  }
  SongList deleted;
  for (const Song &song : requested) {
    if (!failed.count(song.url())) {
      deleted.push_back(song);
    }
  }
  return deleted;
}

inline SongList RemoveDeleted(const SongList &cached, const SongList &deleted) {
  std::set<std::string> removed;
  for (const Song &song : deleted) {
    removed.insert(song.url());
  }
  SongList remaining;
  remaining.reserve(cached.size());
  for (const Song &song : cached) {
    if (!removed.count(song.url())) {
      remaining.push_back(song);
    }
  }
  return remaining;
}

// Qt DeviceView does not rescan the device after delete; it updates the model from MusicStorage.
inline bool ReloadsDeviceAfterDelete() { return false; }

}  // namespace DeviceDeleteJob

#endif
