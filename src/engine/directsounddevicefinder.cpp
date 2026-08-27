#include "engine/directsounddevicefinder.h"

#include <cstdio>

#ifdef _WIN32
#ifdef INTERFACE
#undef INTERFACE
#endif
#include <dsound.h>
#include <objbase.h>
#endif

DirectSoundDeviceFinder::DirectSoundDeviceFinder()
    : DeviceFinder("directsound", {"directsound", "dsound", "directsoundsink", "directx", "directx2", "waveformsink"}) {}

#ifdef _WIN32
int __stdcall DirectSoundDeviceFinder::EnumerateCallback(const void *guid, const char *description, const char *, void *state_voidptr) {
  auto *state = static_cast<State *>(state_voidptr);
  EngineDevice device;
  device.description = description ? description : "";
  if (guid) {
    const GUID *id = static_cast<const GUID *>(guid);
    char text[64];
    snprintf(text, sizeof(text), "{%08lX-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", static_cast<unsigned long>(id->Data1), id->Data2,
             id->Data3, id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3], id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7]);
    device.value = text;
  }
  device.iconname = device.GuessIconName();
  state->devices.push_back(device);
  return 1;
}
#endif

EngineDeviceList DirectSoundDeviceFinder::ListDevices() {
#ifdef _WIN32
  State state;
  DirectSoundEnumerateA(reinterpret_cast<LPDSENUMCALLBACKA>(EnumerateCallback), &state);
  return state.devices;
#else
  return {};
#endif
}
