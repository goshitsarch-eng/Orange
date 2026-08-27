#include "engine/macosdevicefinder.h"

#include <string>

#ifdef __APPLE__
#include <AvailabilityMacros.h>
#include <CoreAudio/AudioHardware.h>
#include <cstdlib>
#include <memory>
#endif

MacOsDeviceFinder::MacOsDeviceFinder() : DeviceFinder("osxaudio", {"osxaudio", "osx", "osxaudiosink"}) {}

EngineDeviceList MacOsDeviceFinder::ListDevices() {
  EngineDeviceList device_list;
#ifdef __APPLE__
  AudioObjectPropertyAddress address = {
      kAudioHardwarePropertyDevices,
      kAudioObjectPropertyScopeGlobal,
#if defined(MAC_OS_VERSION_12_0) && (MAC_OS_X_VERSION_MIN_REQUIRED >= MAC_OS_VERSION_12_0)
      kAudioObjectPropertyElementMain
#else
      kAudioObjectPropertyElementMaster
#endif
  };
  UInt32 size_bytes = 0;
  if (AudioObjectGetPropertyDataSize(kAudioObjectSystemObject, &address, 0, nullptr, &size_bytes) != kAudioHardwareNoError) {
    return device_list;
  }
  std::unique_ptr<AudioDeviceID, decltype(&std::free)> devices(static_cast<AudioDeviceID *>(std::malloc(size_bytes)), &std::free);
  if (!devices || AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size_bytes, devices.get()) != kAudioHardwareNoError) {
    return device_list;
  }
  const UInt32 device_count = size_bytes / sizeof(AudioDeviceID);
  address.mScope = kAudioDevicePropertyScopeOutput;
  for (UInt32 i = 0; i < device_count; ++i) {
    const AudioDeviceID id = devices.get()[i];
    address.mSelector = kAudioDevicePropertyDeviceNameCFString;
    UInt32 name_size = 0;
    if (AudioObjectGetPropertyDataSize(id, &address, 0, nullptr, &name_size) != kAudioHardwareNoError) {
      continue;
    }
    CFStringRef name = nullptr;
    if (AudioObjectGetPropertyData(id, &address, 0, nullptr, &name_size, &name) != kAudioHardwareNoError || !name) {
      continue;
    }
    address.mSelector = kAudioDevicePropertyStreamConfiguration;
    UInt32 buf_size = 0;
    if (AudioObjectGetPropertyDataSize(id, &address, 0, nullptr, &buf_size) != kAudioHardwareNoError) {
      CFRelease(name);
      continue;
    }
    std::unique_ptr<AudioBufferList, decltype(&std::free)> buffers(static_cast<AudioBufferList *>(std::malloc(buf_size)), &std::free);
    if (!buffers || AudioObjectGetPropertyData(id, &address, 0, nullptr, &buf_size, buffers.get()) != kAudioHardwareNoError ||
        buffers->mNumberBuffers == 0) {
      CFRelease(name);
      continue;
    }
    char text[256]{};
    CFStringGetCString(name, text, sizeof(text), kCFStringEncodingUTF8);
    CFRelease(name);
    EngineDevice device;
    device.value = std::to_string(id);
    device.description = text[0] ? text : ("Unknown device " + device.value);
    device.iconname = device.GuessIconName();
    device_list.push_back(device);
  }
#endif
  return device_list;
}
