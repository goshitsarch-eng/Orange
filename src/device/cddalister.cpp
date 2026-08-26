#include "device/cddalister.h"

#include "config.h"
#include "device/cddahelpers.h"

#ifdef HAVE_AUDIOCD
#include <cdio/cdio.h>
#endif

std::vector<ConnectedDevice> CddaLister::List() const {
  std::vector<ConnectedDevice> devices;
#ifdef HAVE_AUDIOCD
  CddaHelpers::EnsureInit();
  char **drives = cdio_get_devices(DRIVER_DEVICE);
  if (!drives) {
    return devices;
  }
  for (char **drive = drives; *drive; ++drive) {
    if (CddaHelpers::ShouldSkipDevice(*drive)) {
      continue;
    }
    ConnectedDevice device;
    device.backend = "cdda";
    device.friendly_name = "Audio CD";
    device.unique_id = std::string("cdda:") + *drive;
    device.mount_path = *drive;
    device.icon = "media-optical-symbolic";
    devices.push_back(device);
  }
  cdio_free_device_list(drives);
#endif
  return devices;
}
