#include "engine/devicefinders.h"

#include "config.h"

#ifdef HAVE_PULSE
#  include <pulse/pulseaudio.h>
#endif

#ifdef HAVE_ALSA
#  include <alsa/asoundlib.h>
#endif

void DeviceFinders::Init() {
  devices_.clear();
  devices_.push_back({"", "Default", "audio-card-symbolic", "autoaudiosink"});

#ifdef HAVE_PULSE
  devices_.push_back({"", "PulseAudio", "audio-card-symbolic", "pulsesink"});
#endif
#ifdef HAVE_ALSA
  void **hints = nullptr;
  if (snd_device_name_hint(-1, "pcm", &hints) == 0) {
    for (void **hint = hints; *hint; ++hint) {
      char *name = snd_device_name_get_hint(*hint, "NAME");
      char *desc = snd_device_name_get_hint(*hint, "DESC");
      if (name) {
        AudioDevice device;
        device.id = name;
        device.description = desc ? desc : name;
        device.iconname = "audio-card-symbolic";
        device.output = "alsasink";
        devices_.push_back(device);
      }
      free(name);
      free(desc);
    }
    snd_device_name_free_hint(hints);
  }
#endif
  devices_.push_back({"", "PipeWire", "audio-card-symbolic", "pipewiresink"});
}

std::vector<AudioDevice> DeviceFinders::ListDevices() const { return devices_; }

std::vector<std::string> DeviceFinders::Outputs() const {
  return {"autoaudiosink", "pulsesink", "pipewiresink", "alsasink"};
}
