#include "device/macosdevicelister.h"

#include <string>

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#endif

MacOsDeviceLister::MacOsDeviceLister() {
#ifdef __APPLE__
  Init();
#endif
}

MacOsDeviceLister::~MacOsDeviceLister() {
#ifdef __APPLE__
  Shutdown();
#endif
}

#ifdef __APPLE__
void MacOsDeviceLister::Init() {
  if (session_) {
    return;
  }
  session_ = DASessionCreate(kCFAllocatorDefault);
  if (!session_) {
    return;
  }
  DARegisterDiskAppearedCallback(session_, nullptr, DiskAdded, this);
  DARegisterDiskDisappearedCallback(session_, nullptr, DiskRemoved, this);
}

void MacOsDeviceLister::Shutdown() {
  if (!session_) {
    return;
  }
  DAUnregisterCallback(session_, reinterpret_cast<void *>(DiskAdded), this);
  DAUnregisterCallback(session_, reinterpret_cast<void *>(DiskRemoved), this);
  CFRelease(session_);
  session_ = nullptr;
}

void MacOsDeviceLister::DiskAdded(DADiskRef disk, void *context) {
  auto *self = static_cast<MacOsDeviceLister *>(context);
  CFDictionaryRef desc = DADiskCopyDescription(disk);
  if (!desc) {
    return;
  }
  const void *volume_name = CFDictionaryGetValue(desc, kDADiskDescriptionVolumeNameKey);
  const void *bsd = CFDictionaryGetValue(desc, kDADiskDescriptionMediaBSDNameKey);
  const void *removable = CFDictionaryGetValue(desc, kDADiskDescriptionMediaRemovableKey);
  const bool is_removable = removable && CFBooleanGetValue(static_cast<CFBooleanRef>(removable));
  if (!is_removable) {
    CFRelease(desc);
    return;
  }
  ConnectedDevice device;
  char name[256]{};
  if (volume_name) {
    CFStringGetCString(static_cast<CFStringRef>(volume_name), name, sizeof(name), kCFStringEncodingUTF8);
  }
  char bsd_name[64]{};
  if (bsd) {
    CFStringGetCString(static_cast<CFStringRef>(bsd), bsd_name, sizeof(bsd_name), kCFStringEncodingUTF8);
  }
  device.friendly_name = name[0] ? name : bsd_name;
  device.unique_id = bsd_name[0] ? bsd_name : device.friendly_name;
  device.backend = "macos";
  device.icon = "drive-removable-media-symbolic";
  device.mount_path = std::string("/Volumes/") + device.friendly_name;
  {
    std::lock_guard<std::mutex> lock(self->mutex_);
    self->devices_[device.unique_id] = device;
  }
  CFRelease(desc);
}

void MacOsDeviceLister::DiskRemoved(DADiskRef disk, void *context) {
  auto *self = static_cast<MacOsDeviceLister *>(context);
  CFDictionaryRef desc = DADiskCopyDescription(disk);
  if (!desc) {
    return;
  }
  const void *bsd = CFDictionaryGetValue(desc, kDADiskDescriptionMediaBSDNameKey);
  char bsd_name[64]{};
  if (bsd) {
    CFStringGetCString(static_cast<CFStringRef>(bsd), bsd_name, sizeof(bsd_name), kCFStringEncodingUTF8);
  }
  {
    std::lock_guard<std::mutex> lock(self->mutex_);
    self->devices_.erase(bsd_name);
  }
  CFRelease(desc);
}
#endif

std::vector<ConnectedDevice> MacOsDeviceLister::List() const {
#ifdef __APPLE__
  std::lock_guard<std::mutex> lock(mutex_);
  std::vector<ConnectedDevice> out;
  out.reserve(devices_.size());
  for (const auto &entry : devices_) {
    out.push_back(entry.second);
  }
  return out;
#else
  return {};
#endif
}
