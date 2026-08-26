#include "device/cddalister.h"

#include "config.h"
#include "device/cddadevice.h"

#ifdef HAVE_AUDIOCD
#include <cdio/cdio.h>
#endif

std::vector<ConnectedDevice> CddaLister::List() const {
  std::vector<ConnectedDevice> devices;
#ifdef HAVE_AUDIOCD
  char **drives = cdio_get_devices(DRIVER_DEVICE);
  if (!drives) {
    CddaDevice cd;
    if (!cd.Songs().empty()) {
      ConnectedDevice entry = cd.info();
      entry.unique_id = "cdda:default";
      devices.push_back(entry);
    }
    return devices;
  }
  for (char **drive = drives; *drive; ++drive) {
    ConnectedDevice device;
    device.backend = "cdda";
    device.friendly_name = "Audio CD";
    device.unique_id = std::string("cdda:") + *drive;
    device.mount_path = *drive;
    device.icon = "media-optical-symbolic";
    devices.push_back(device);
  }
  cdio_free_device_list(drives);
#else
  CddaDevice cd;
  if (!cd.Songs().empty()) {
    ConnectedDevice entry = cd.info();
    entry.unique_id = "cdda:default";
    devices.push_back(entry);
  }
#endif
  return devices;
}
