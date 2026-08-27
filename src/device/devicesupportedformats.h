#ifndef STRAWBERRY_DEVICESUPPORTEDFORMATS_H
#define STRAWBERRY_DEVICESUPPORTEDFORMATS_H

#include "core/song.h"
#include "device/connecteddevice.h"
#include "organize/organizetranscode.h"

#include <algorithm>
#include <string>
#include <vector>

namespace DeviceSupportedFormats {

enum class Page {
  Formats,
  NotConnected,
  Loading,
};

inline const char *QueryingDevice() { return "Querying device..."; }

inline const char *SupportedFormatsIntro() { return "This device supports the following file formats:"; }

// Qt GPodDevice::GetSupportedFiletypes
inline std::vector<Song::FileType> GPodFormats() { return {Song::FileType::MP4, Song::FileType::MPEG, Song::FileType::ALAC}; }

inline bool PhysicallyPresent(const ConnectedDevice &device) { return !device.unique_id.empty() && !device.remembered; }

inline bool IsMtp(const ConnectedDevice &device) { return device.backend == "mtp"; }

inline bool Opened(const ConnectedDevice &device) {
  return !device.mount_path.empty() || (IsMtp(device) && PhysicallyPresent(device));
}

// Qt: Open device is enabled when a lister exists but the device is not connected yet.
inline bool OpenEnabled(const ConnectedDevice &device) { return PhysicallyPresent(device) && !Opened(device); }

inline bool ShouldQuery(const ConnectedDevice &device) { return IsMtp(device) && Opened(device); }

inline Page PageFor(const ConnectedDevice &device, bool query_finished = false) {
  if (!PhysicallyPresent(device) || !Opened(device)) {
    return Page::NotConnected;
  }
  if (ShouldQuery(device) && !query_finished) {
    return Page::Loading;
  }
  return Page::Formats;
}

inline const char *StackName(Page page) {
  switch (page) {
    case Page::Loading:
      return "loading";
    case Page::Formats:
      return "formats";
    case Page::NotConnected:
    default:
      return "not-connected";
  }
}

inline std::vector<Song::FileType> Unique(const std::vector<Song::FileType> &types) {
  std::vector<Song::FileType> out;
  for (Song::FileType type : types) {
    if (!OrganizeTranscode::Contains(out, type)) {
      out.push_back(type);
    }
  }
  return out;
}

// Qt UpdateFormatsFinished: a failed query clears the list. gPod uses a fixed list. Filesystem reports none.
inline std::vector<Song::FileType> Resolve(const std::string &backend, const std::vector<Song::FileType> &queried, bool query_ok,
                                           bool queried_attempted) {
  if (queried_attempted) {
    return query_ok ? Unique(queried) : std::vector<Song::FileType>{};
  }
  if (backend == "gpod") {
    return GPodFormats();
  }
  return {};
}

}  // namespace DeviceSupportedFormats

#endif
