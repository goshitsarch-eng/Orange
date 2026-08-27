#include "engine/devicefinders.h"

#include "config.h"
#ifdef HAVE_ALSA
#include "engine/alsadevicefinder.h"
#include "engine/alsapcmdevicefinder.h"
#endif
#ifdef HAVE_PULSE
#include "engine/pulsedevicefinder.h"
#endif

DeviceFinders::DeviceFinders() = default;
DeviceFinders::~DeviceFinders() = default;

void DeviceFinders::Init() {
  finders_.clear();
  devices_.clear();
  devices_.push_back({"", "Default", "audio-card-symbolic", "autoaudiosink"});
#ifdef HAVE_PULSE
  finders_.push_back(std::make_unique<PulseDeviceFinder>());
#endif
#ifdef HAVE_ALSA
  finders_.push_back(std::make_unique<AlsaDeviceFinder>());
  finders_.push_back(std::make_unique<AlsaPCMDeviceFinder>());
#endif
  for (auto &finder : finders_) {
    if (!finder->Initialize()) {
      continue;
    }
    for (const EngineDevice &device : finder->ListDevices()) {
      AudioDevice audio;
      audio.id = device.value;
      audio.description = device.description.empty() ? device.value : device.description;
      audio.iconname = device.iconname.empty() ? "audio-card-symbolic" : device.iconname;
      audio.output = finder->outputs().empty() ? "autoaudiosink" : finder->outputs().back();
      devices_.push_back(audio);
    }
  }
  devices_.push_back({"", "PipeWire", "audio-card-symbolic", "pipewiresink"});
}

std::vector<DeviceFinder *> DeviceFinders::ListFinders() const {
  std::vector<DeviceFinder *> out;
  for (const auto &finder : finders_) {
    out.push_back(finder.get());
  }
  return out;
}

std::vector<AudioDevice> DeviceFinders::ListDevices() const { return devices_; }

std::vector<std::string> DeviceFinders::Outputs() const {
  return {"autoaudiosink", "pulsesink", "pipewiresink", "alsasink"};
}

std::string DeviceFinders::ChoiceKey(const std::string &output, const std::string &device) { return output + "|" + device; }

void DeviceFinders::SplitChoiceKey(const std::string &key, std::string *output, std::string *device) {
  const auto pos = key.find('|');
  if (pos == std::string::npos) {
    if (output) {
      *output = key;
    }
    if (device) {
      device->clear();
    }
    return;
  }
  if (output) {
    *output = key.substr(0, pos);
  }
  if (device) {
    *device = key.substr(pos + 1);
  }
}

std::string DeviceFinders::OutputLabel(const std::string &output) {
  if (output == "autoaudiosink") {
    return "Automatic";
  }
  if (output == "pulsesink") {
    return "PulseAudio";
  }
  if (output == "pipewiresink") {
    return "PipeWire";
  }
  if (output == "alsasink") {
    return "ALSA";
  }
  return output;
}
