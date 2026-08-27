#ifndef STRAWBERRY_PLATFORMDEVICEOUTPUTS_H
#define STRAWBERRY_PLATFORMDEVICEOUTPUTS_H

#include <string>
#include <vector>

namespace PlatformDeviceOutputs {

inline const char *DefaultDeviceDescription() { return "Default device"; }

inline std::vector<const char *> MMDeviceOutputs() { return {"wasapisink", "wasapi2sink"}; }

inline std::vector<const char *> DirectSoundOutputs() {
  return {"directsound", "dsound", "directsoundsink", "directx", "directx2", "waveformsink"};
}

inline std::vector<const char *> AsioOutputs() { return {"asiosink"}; }

inline std::vector<const char *> UwpOutputs() { return {"wasapi2sink_"}; }

inline std::vector<const char *> MacOsOutputs() { return {"osxaudio", "osx", "osxaudiosink"}; }

inline std::vector<const char *> ExtraSinks() {
#ifdef _WIN32
  return {"wasapisink", "wasapi2sink", "directsoundsink", "asiosink"};
#elif defined(__APPLE__)
  return {"osxaudiosink"};
#else
  return {};
#endif
}

inline const char *OutputLabel(const char *output) {
  if (!output) {
    return "";
  }
  const std::string name = output;
  if (name == "wasapisink" || name == "wasapi2sink" || name == "wasapi2sink_") {
    return "WASAPI";
  }
  if (name == "directsoundsink" || name == "directsound" || name == "dsound") {
    return "DirectSound";
  }
  if (name == "asiosink") {
    return "ASIO";
  }
  if (name == "osxaudiosink" || name == "osxaudio" || name == "osx") {
    return "Core Audio";
  }
  return "";
}

}  // namespace PlatformDeviceOutputs

#endif
