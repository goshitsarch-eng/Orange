#include "engine/alsapcmdevicefinder.h"

#include "config.h"

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#include <cstring>
#endif

AlsaPCMDeviceFinder::AlsaPCMDeviceFinder() : DeviceFinder("alsa", {"alsa", "alsasink"}) {}

EngineDeviceList AlsaPCMDeviceFinder::ListDevices() {
  EngineDeviceList devices;
#ifdef HAVE_ALSA
  void **hints = nullptr;
  if (snd_device_name_hint(-1, "pcm", &hints) < 0) {
    return devices;
  }
  for (void **n = hints; *n; ++n) {
    char *hint_io = snd_device_name_get_hint(*n, "IOID");
    char *hint_name = snd_device_name_get_hint(*n, "NAME");
    char *hint_desc = snd_device_name_get_hint(*n, "DESC");
    if (hint_name && (!hint_io || strcmp(hint_io, "Output") == 0)) {
      EngineDevice device;
      device.value = hint_name;
      if (hint_desc) {
        std::string description;
        const char *last = hint_desc;
        for (char *p = hint_desc; *p; ++p) {
          if (*p == '\n') {
            *p = '\0';
            if (!description.empty()) {
              description.push_back(' ');
            }
            description += last;
            last = p + 1;
          }
        }
        if (last && *last) {
          if (!description.empty()) {
            description.push_back(' ');
          }
          description += last;
        }
        device.description = description.empty() ? hint_name : description;
      } else {
        device.description = hint_name;
      }
      device.iconname = device.GuessIconName();
      devices.push_back(device);
    }
    free(hint_io);
    free(hint_name);
    free(hint_desc);
  }
  snd_device_name_free_hint(hints);
#endif
  return devices;
}
