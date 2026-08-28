#include "device/macosdevicelister.h"

#include "core/logging.h"
#include "device/macosdeviceclassify.h"

#include <cstring>
#include <string>
#ifdef __APPLE__
#include <cstdint>
#include <sys/param.h>
#endif

#ifdef __APPLE__
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/usb/IOUSBLib.h>
#include <IOKit/usb/USB.h>
#ifdef HAVE_AUDIOCD
#include <IOKit/storage/IOCDMedia.h>
#endif
#ifdef HAVE_MTP
#include "device/devicecopyjob.h"
#include "device/mtpconnection.h"
#include <libmtp.h>
#endif
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

namespace {

std::string CFStringToStd(CFStringRef value) {
  if (!value) {
    return {};
  }
  char buf[256]{};
  CFStringGetCString(value, buf, sizeof(buf), kCFStringEncodingUTF8);
  return buf;
}

std::string CopyCFStringProperty(io_object_t device, CFStringRef key) {
  CFTypeRef value = IORegistryEntryCreateCFProperty(device, key, kCFAllocatorDefault, 0);
  if (!value) {
    return {};
  }
  std::string out;
  if (CFGetTypeID(value) == CFStringGetTypeID()) {
    out = CFStringToStd(static_cast<CFStringRef>(value));
  }
  CFRelease(value);
  return out;
}

uint16_t CopyCFNumberProperty(io_object_t device, CFStringRef key) {
  CFTypeRef value = IORegistryEntryCreateCFProperty(device, key, kCFAllocatorDefault, 0);
  if (!value) {
    return 0;
  }
  uint16_t out = 0;
  if (CFGetTypeID(value) == CFNumberGetTypeID()) {
    int n = 0;
    CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &n);
    out = static_cast<uint16_t>(n);
  }
  CFRelease(value);
  return out;
}

int CopyCFIntProperty(io_object_t device, CFStringRef key) {
  CFTypeRef value = IORegistryEntryCreateCFProperty(device, key, kCFAllocatorDefault, 0);
  if (!value) {
    return -1;
  }
  int out = -1;
  if (CFGetTypeID(value) == CFNumberGetTypeID()) {
    CFNumberGetValue(static_cast<CFNumberRef>(value), kCFNumberIntType, &out);
  }
  CFRelease(value);
  return out;
}

mach_port_t IOKitMasterPort() {
#if defined(MAC_OS_VERSION_12_0) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_12_0)
  return kIOMainPortDefault;
#else
  return kIOMasterPortDefault;
#endif
}

}  // namespace

void MacOsDeviceLister::Init() {
  if (session_) {
    return;
  }
#ifdef HAVE_MTP
  LoadSupportedMtpDevices();
  MtpConnection::InitLibMtp();
#endif

  session_ = DASessionCreate(kCFAllocatorDefault);
  if (!session_) {
    return;
  }
  DARegisterDiskAppearedCallback(session_, kDADiskDescriptionMatchVolumeMountable, DiskAdded, this);
  DARegisterDiskDisappearedCallback(session_, nullptr, DiskRemoved, this);
  DASessionScheduleWithRunLoop(session_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);

  notify_port_ = IONotificationPortCreate(IOKitMasterPort());
  if (!notify_port_) {
    return;
  }
  CFRunLoopSourceRef source = IONotificationPortGetRunLoopSource(notify_port_);
  if (source) {
    CFRunLoopAddSource(CFRunLoopGetMain(), source, kCFRunLoopDefaultMode);
  }

  CFMutableDictionaryRef matching = IOServiceMatching(kIOUSBDeviceClassName);
  CFRetain(matching);
  kern_return_t err = IOServiceAddMatchingNotification(notify_port_, kIOFirstMatchNotification, matching, USBDeviceAdded, this, &usb_added_);
  if (err == KERN_SUCCESS) {
    USBDeviceAdded(this, usb_added_);
  } else {
    LogWarning("MacOsDeviceLister: could not watch USB device connection");
  }

  err = IOServiceAddMatchingNotification(notify_port_, kIOTerminatedNotification, matching, USBDeviceRemoved, this, &usb_removed_);
  if (err == KERN_SUCCESS) {
    USBDeviceRemoved(this, usb_removed_);
  } else {
    LogWarning("MacOsDeviceLister: could not watch USB device removal");
  }
}

void MacOsDeviceLister::Shutdown() {
  if (notify_port_) {
    CFRunLoopSourceRef source = IONotificationPortGetRunLoopSource(notify_port_);
    if (source) {
      CFRunLoopRemoveSource(CFRunLoopGetMain(), source, kCFRunLoopDefaultMode);
    }
    IONotificationPortDestroy(notify_port_);
    notify_port_ = nullptr;
  }
  if (usb_added_) {
    IOObjectRelease(usb_added_);
    usb_added_ = 0;
  }
  if (usb_removed_) {
    IOObjectRelease(usb_removed_);
    usb_removed_ = 0;
  }
  if (!session_) {
    return;
  }
  DAUnregisterCallback(session_, reinterpret_cast<void *>(DiskAdded), this);
  DAUnregisterCallback(session_, reinterpret_cast<void *>(DiskRemoved), this);
  DASessionUnscheduleFromRunLoop(session_, CFRunLoopGetMain(), kCFRunLoopDefaultMode);
  CFRelease(session_);
  session_ = nullptr;
}

#ifdef HAVE_MTP
void MacOsDeviceLister::LoadSupportedMtpDevices() {
  if (!supported_mtp_.empty()) {
    return;
  }
  LIBMTP_device_entry_t *devices = nullptr;
  int num = 0;
  if (LIBMTP_Get_Supported_Devices_List(&devices, &num) == 0 && devices) {
    supported_mtp_.reserve(static_cast<size_t>(num) + 1);
    for (int i = 0; i < num; ++i) {
      MTPDevice entry;
      entry.vendor = devices[i].vendor ? devices[i].vendor : "";
      entry.product = devices[i].product ? devices[i].product : "";
      entry.vendor_id = devices[i].vendor_id;
      entry.product_id = devices[i].product_id;
      entry.quirks = static_cast<int>(devices[i].device_flags);
      supported_mtp_.push_back(entry);
    }
  } else {
    LogWarning("MacOsDeviceLister: failed to get MTP device list");
  }
  MTPDevice sandisk;
  sandisk.vendor = "SanDisk";
  sandisk.product = "Sansa Clip+";
  sandisk.vendor_id = 0x781;
  sandisk.product_id = 0x74d0;
  sandisk.quirks = 0x2 | 0x4 | 0x40 | 0x4000;
  supported_mtp_.push_back(sandisk);
}

uint64_t MacOsDeviceLister::GetCapacity(const std::string &serial) {
  MtpConnection connection;
  if (!connection.OpenBySerial(MacOsDeviceClassify::MTPSerial(serial)) || !connection.device() || !connection.device()->storage) {
    return 0;
  }
  uint64_t capacity = 0;
  for (LIBMTP_devicestorage_t *storage = connection.device()->storage; storage; storage = storage->next) {
    capacity += storage->MaxCapacity;
  }
  return capacity;
}

void MacOsDeviceLister::FoundMTPDevice(const MTPDevice &device, const std::string &serial) {
  const std::string unique_id = MacOsDeviceClassify::MTPUniqueId(MacOsDeviceClassify::MTPSerial(serial));
  ConnectedDevice entry;
  entry.unique_id = unique_id;
  entry.friendly_name = MacOsDeviceClassify::FriendlyName(device.vendor, device.product);
  entry.backend = MacOsDeviceClassify::BackendForKind(false, true);
  entry.icon = MacOsDeviceClassify::IconForKind(false, true);
  entry.size = static_cast<int64_t>(GetCapacity(unique_id));
  {
    std::lock_guard<std::mutex> lock(mutex_);
    mtp_devices_[unique_id] = device;
    devices_[unique_id] = entry;
  }
  LogDebug("MacOsDeviceLister: MTP device %s", unique_id.c_str());
  DevicesChanged.Emit();
}

void MacOsDeviceLister::RemovedMTPDevice(const std::string &serial) {
  const std::string unique_id = MacOsDeviceClassify::MTPUniqueId(MacOsDeviceClassify::MTPSerial(serial));
  bool removed = false;
  {
    std::lock_guard<std::mutex> lock(mutex_);
    removed = mtp_devices_.erase(unique_id) > 0;
    devices_.erase(unique_id);
  }
  if (removed) {
    LogDebug("MacOsDeviceLister: MTP device removed %s", unique_id.c_str());
    DevicesChanged.Emit();
  }
}
#endif

void MacOsDeviceLister::DiskAdded(DADiskRef disk, void *context) {
  auto *self = static_cast<MacOsDeviceLister *>(context);
  CFDictionaryRef desc = DADiskCopyDescription(disk);
  if (!desc) {
    return;
  }

  char kind_buf[64]{};
  const void *kind = CFDictionaryGetValue(desc, kDADiskDescriptionMediaKindKey);
  if (kind && CFGetTypeID(static_cast<CFTypeRef>(kind)) == CFStringGetTypeID()) {
    CFStringGetCString(static_cast<CFStringRef>(kind), kind_buf, sizeof(kind_buf), kCFStringEncodingUTF8);
  }
  const bool is_cd = MacOsDeviceClassify::IsCDKind(kind_buf);

  const void *removable = CFDictionaryGetValue(desc, kDADiskDescriptionMediaRemovableKey);
  const bool is_removable = removable && CFBooleanGetValue(static_cast<CFBooleanRef>(removable));
  if (!is_cd && !is_removable) {
    CFRelease(desc);
    return;
  }

  ConnectedDevice device;
  char name[256]{};
  const void *volume_name = CFDictionaryGetValue(desc, kDADiskDescriptionVolumeNameKey);
  if (volume_name && CFGetTypeID(static_cast<CFTypeRef>(volume_name)) == CFStringGetTypeID()) {
    CFStringGetCString(static_cast<CFStringRef>(volume_name), name, sizeof(name), kCFStringEncodingUTF8);
  }
  char bsd_name[64]{};
  const char *bsd = DADiskGetBSDName(disk);
  if (bsd) {
    strncpy(bsd_name, bsd, sizeof(bsd_name) - 1);
  } else {
    const void *bsd_value = CFDictionaryGetValue(desc, kDADiskDescriptionMediaBSDNameKey);
    if (bsd_value && CFGetTypeID(static_cast<CFTypeRef>(bsd_value)) == CFStringGetTypeID()) {
      CFStringGetCString(static_cast<CFStringRef>(bsd_value), bsd_name, sizeof(bsd_name), kCFStringEncodingUTF8);
    }
  }

  device.unique_id = bsd_name[0] ? bsd_name : (name[0] ? name : "disk");
  device.friendly_name = name[0] ? name : (is_cd ? "Audio CD" : device.unique_id);
  device.backend = MacOsDeviceClassify::BackendForKind(is_cd, false);
  device.icon = MacOsDeviceClassify::IconForKind(is_cd, false);
  if (is_cd) {
    device.mount_path = MacOsDeviceClassify::RawDevicePath(bsd_name);
  } else {
    const void *volume_path = CFDictionaryGetValue(desc, kDADiskDescriptionVolumePathKey);
    if (volume_path && CFGetTypeID(static_cast<CFTypeRef>(volume_path)) == CFURLGetTypeID()) {
      char path[PATH_MAX]{};
      if (CFURLGetFileSystemRepresentation(static_cast<CFURLRef>(volume_path), true, reinterpret_cast<UInt8 *>(path), sizeof(path))) {
        device.mount_path = path;
      }
    }
    if (device.mount_path.empty()) {
      device.mount_path = std::string("/Volumes/") + device.friendly_name;
    }
  }
  const void *size = CFDictionaryGetValue(desc, kDADiskDescriptionMediaSizeKey);
  if (size && CFGetTypeID(static_cast<CFTypeRef>(size)) == CFNumberGetTypeID()) {
    int64_t bytes = 0;
    CFNumberGetValue(static_cast<CFNumberRef>(size), kCFNumberSInt64Type, &bytes);
    device.size = bytes;
  }

  {
    std::lock_guard<std::mutex> lock(self->mutex_);
    self->devices_[device.unique_id] = device;
  }
  CFRelease(desc);
  self->DevicesChanged.Emit();
}

void MacOsDeviceLister::DiskRemoved(DADiskRef disk, void *context) {
  auto *self = static_cast<MacOsDeviceLister *>(context);
  char bsd_name[64]{};
  const char *bsd = DADiskGetBSDName(disk);
  if (bsd) {
    strncpy(bsd_name, bsd, sizeof(bsd_name) - 1);
  } else {
    CFDictionaryRef desc = DADiskCopyDescription(disk);
    if (desc) {
      const void *bsd_value = CFDictionaryGetValue(desc, kDADiskDescriptionMediaBSDNameKey);
      if (bsd_value && CFGetTypeID(static_cast<CFTypeRef>(bsd_value)) == CFStringGetTypeID()) {
        CFStringGetCString(static_cast<CFStringRef>(bsd_value), bsd_name, sizeof(bsd_name), kCFStringEncodingUTF8);
      }
      CFRelease(desc);
    }
  }
  {
    std::lock_guard<std::mutex> lock(self->mutex_);
    self->devices_.erase(bsd_name);
  }
  self->DevicesChanged.Emit();
}

void MacOsDeviceLister::USBDeviceAdded(void *refcon, io_iterator_t it) {
  auto *self = static_cast<MacOsDeviceLister *>(refcon);
  io_object_t object = 0;
  while ((object = IOIteratorNext(it))) {
    CFStringRef class_name = IOObjectCopyClass(object);
    const bool is_usb = class_name && CFStringCompare(class_name, CFSTR(kIOUSBDeviceClassName), 0) == kCFCompareEqualTo;
    if (class_name) {
      CFRelease(class_name);
    }
    if (!is_usb) {
      IOObjectRelease(object);
      continue;
    }

#ifdef HAVE_MTP
    const uint16_t vendor_id = CopyCFNumberProperty(object, CFSTR(kUSBVendorID));
    const uint16_t product_id = CopyCFNumberProperty(object, CFSTR(kUSBProductID));
    const int device_class = CopyCFIntProperty(object, CFSTR("bDeviceClass"));
    if (MacOsDeviceClassify::ShouldSkipUsbDevice(vendor_id, product_id, device_class)) {
      IOObjectRelease(object);
      continue;
    }

    MTPDevice mtp_device;
    mtp_device.vendor = CopyCFStringProperty(object, CFSTR(kUSBVendorString));
    mtp_device.product = CopyCFStringProperty(object, CFSTR(kUSBProductString));
    mtp_device.vendor_id = vendor_id;
    mtp_device.product_id = product_id;
    mtp_device.address = CopyCFIntProperty(object, CFSTR("USB Address"));
    const std::string usb_serial = CopyCFStringProperty(object, CFSTR(kUSBSerialNumberString));
    const std::string serial = usb_serial.empty()
                                   ? std::to_string(vendor_id) + ":" + std::to_string(product_id) + ":" + std::to_string(mtp_device.address)
                                   : usb_serial;

    bool matched = false;
    for (const MTPDevice &supported : self->supported_mtp_) {
      if (MacOsDeviceClassify::MatchesMtpDevice(vendor_id, product_id, supported.vendor_id, supported.product_id)) {
        mtp_device.quirks = supported.quirks;
        if (mtp_device.vendor.empty()) {
          mtp_device.vendor = supported.vendor;
        }
        if (mtp_device.product.empty()) {
          mtp_device.product = supported.product;
        }
        matched = true;
        break;
      }
    }
    if (matched) {
      self->FoundMTPDevice(mtp_device, MacOsDeviceClassify::MTPUniqueId(serial));
    }
#else
    (void)self;
#endif
    IOObjectRelease(object);
  }
}

void MacOsDeviceLister::USBDeviceRemoved(void *refcon, io_iterator_t it) {
  auto *self = static_cast<MacOsDeviceLister *>(refcon);
  io_object_t object = 0;
  while ((object = IOIteratorNext(it))) {
#ifdef HAVE_MTP
    const std::string usb_serial = CopyCFStringProperty(object, CFSTR(kUSBSerialNumberString));
    const uint16_t vendor_id = CopyCFNumberProperty(object, CFSTR(kUSBVendorID));
    const uint16_t product_id = CopyCFNumberProperty(object, CFSTR(kUSBProductID));
    const int address = CopyCFIntProperty(object, CFSTR("USB Address"));
    const std::string serial = usb_serial.empty()
                                   ? std::to_string(vendor_id) + ":" + std::to_string(product_id) + ":" + std::to_string(address)
                                   : usb_serial;
    self->RemovedMTPDevice(MacOsDeviceClassify::MTPUniqueId(serial));
#else
    (void)self;
#endif
    IOObjectRelease(object);
  }
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
