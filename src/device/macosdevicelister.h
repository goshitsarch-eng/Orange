#ifndef STRAWBERRY_MACOSDEVICELISTER_H
#define STRAWBERRY_MACOSDEVICELISTER_H

#include "config.h"
#include "core/signal.h"
#include "device/devicelister.h"

#ifdef __APPLE__
#include <DiskArbitration/DiskArbitration.h>
#include <IOKit/IOKitLib.h>
#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <vector>
#endif

class MacOsDeviceLister : public DeviceLister {
 public:
  MacOsDeviceLister();
  ~MacOsDeviceLister() override;

  std::string backend() const override { return "macos"; }
  std::vector<ConnectedDevice> List() const override;

  Signal<> DevicesChanged;

#ifdef __APPLE__
  void Init();
  void Shutdown();

#ifdef HAVE_MTP
  struct MTPDevice {
    std::string vendor;
    std::string product;
    uint16_t vendor_id = 0;
    uint16_t product_id = 0;
    int quirks = 0;
    int bus = -1;
    int address = -1;
    uint64_t capacity = 0;
    uint64_t free_space = 0;
  };
#endif

 private:
  static void DiskAdded(DADiskRef disk, void *context);
  static void DiskRemoved(DADiskRef disk, void *context);
  static void USBDeviceAdded(void *refcon, io_iterator_t it);
  static void USBDeviceRemoved(void *refcon, io_iterator_t it);

#ifdef HAVE_MTP
  void FoundMTPDevice(const MTPDevice &device, const std::string &serial);
  void RemovedMTPDevice(const std::string &serial);
  uint64_t GetCapacity(const std::string &serial);
  void LoadSupportedMtpDevices();
#endif

  DASessionRef session_ = nullptr;
  IONotificationPortRef notify_port_ = nullptr;
  io_iterator_t usb_added_ = 0;
  io_iterator_t usb_removed_ = 0;
  mutable std::mutex mutex_;
  std::map<std::string, ConnectedDevice> devices_;
#ifdef HAVE_MTP
  std::map<std::string, MTPDevice> mtp_devices_;
  std::vector<MTPDevice> supported_mtp_;
#endif
#endif
};

#endif
