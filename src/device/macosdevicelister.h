#ifndef STRAWBERRY_MACOSDEVICELISTER_H
#define STRAWBERRY_MACOSDEVICELISTER_H

#include "device/devicelister.h"

#ifdef __APPLE__
#include <DiskArbitration/DiskArbitration.h>
#include <map>
#include <mutex>
#endif

class MacOsDeviceLister : public DeviceLister {
 public:
  MacOsDeviceLister();
  ~MacOsDeviceLister() override;

  std::string backend() const override { return "macos"; }
  std::vector<ConnectedDevice> List() const override;

#ifdef __APPLE__
  void Init();
  void Shutdown();

 private:
  static void DiskAdded(DADiskRef disk, void *context);
  static void DiskRemoved(DADiskRef disk, void *context);

  DASessionRef session_ = nullptr;
  mutable std::mutex mutex_;
  std::map<std::string, ConnectedDevice> devices_;
#endif
};

#endif
