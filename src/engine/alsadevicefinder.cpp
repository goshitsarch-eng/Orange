#include "engine/alsadevicefinder.h"

#include "config.h"
#include "core/logging.h"

#ifdef HAVE_ALSA
#include <alsa/asoundlib.h>
#include <cstdio>
#include <cerrno>
#endif

AlsaDeviceFinder::AlsaDeviceFinder() : DeviceFinder("alsa", {"alsa", "alsasink"}) {}

EngineDeviceList AlsaDeviceFinder::ListDevices() {
  EngineDeviceList devices;
#ifdef HAVE_ALSA
  int card = -1;
  snd_ctl_card_info_t *cardinfo = nullptr;
  snd_ctl_card_info_alloca(&cardinfo);
  while (true) {
    int result = snd_card_next(&card);
    if (result < 0 || card < 0) {
      break;
    }
    char str[32];
    snprintf(str, sizeof(str), "hw:%d", card);
    snd_ctl_t *handle = nullptr;
    result = snd_ctl_open(&handle, str, 0);
    if (result < 0) {
      continue;
    }
    result = snd_ctl_card_info(handle, cardinfo);
    if (result < 0) {
      snd_ctl_close(handle);
      continue;
    }
    int dev = -1;
    snd_pcm_info_t *pcminfo = nullptr;
    snd_pcm_info_alloca(&pcminfo);
    while (true) {
      result = snd_ctl_pcm_next_device(handle, &dev);
      if (result < 0 || dev < 0) {
        break;
      }
      snd_pcm_info_set_device(pcminfo, dev);
      snd_pcm_info_set_subdevice(pcminfo, 0);
      snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
      result = snd_ctl_pcm_info(handle, pcminfo);
      if (result < 0) {
        continue;
      }
      EngineDevice device;
      device.description = std::string(snd_ctl_card_info_get_name(cardinfo)) + " " + snd_pcm_info_get_name(pcminfo);
      device.card = card;
      device.device = dev;
      device.value = std::string("hw:") + snd_ctl_card_info_get_id(cardinfo) + "," + std::to_string(dev);
      device.iconname = device.GuessIconName();
      devices.push_back(device);
      device.value = std::string("plughw:") + snd_ctl_card_info_get_id(cardinfo) + "," + std::to_string(dev);
      devices.push_back(device);
    }
    snd_ctl_close(handle);
  }
  snd_config_update_free_global();
#else
  (void)devices;
#endif
  return devices;
}
