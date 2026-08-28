#include "engine/pulsedevicefinder.h"

#include "core/logging.h"

#include <glib.h>

PulseDeviceFinder::PulseDeviceFinder() : DeviceFinder("pulseaudio", {"pulseaudio", "pulse", "pulsesink"}) {}

PulseDeviceFinder::~PulseDeviceFinder() {
#ifdef HAVE_PULSE
  if (context_) {
    pa_context_disconnect(context_);
    pa_context_unref(context_);
  }
  if (mainloop_) {
    pa_mainloop_free(mainloop_);
  }
#endif
}

bool PulseDeviceFinder::Initialize() {
#ifdef HAVE_PULSE
  mainloop_ = pa_mainloop_new();
  if (!mainloop_) {
    LogWarning("Failed to create pulseaudio mainloop");
    return false;
  }
  return Reconnect();
#else
  return false;
#endif
}

#ifdef HAVE_PULSE
bool PulseDeviceFinder::Reconnect() {
  if (context_) {
    pa_context_disconnect(context_);
    pa_context_unref(context_);
    context_ = nullptr;
  }
  context_ = pa_context_new(pa_mainloop_get_api(mainloop_), "Orange device finder");
  if (!context_) {
    return false;
  }
  if (pa_context_connect(context_, nullptr, PA_CONTEXT_NOFLAGS, nullptr) < 0) {
    pa_context_unref(context_);
    context_ = nullptr;
    return false;
  }
  for (int i = 0; i < 200; ++i) {
    const pa_context_state_t state = pa_context_get_state(context_);
    if (state == PA_CONTEXT_FAILED || state == PA_CONTEXT_TERMINATED) {
      pa_context_disconnect(context_);
      pa_context_unref(context_);
      context_ = nullptr;
      return false;
    }
    if (state == PA_CONTEXT_READY) {
      return true;
    }
    pa_mainloop_iterate(mainloop_, 0, nullptr);
    g_usleep(10000);
  }
  return false;
}

void PulseDeviceFinder::GetSinkInfoCallback(pa_context *, const pa_sink_info *info, int eol, void *state_voidptr) {
  auto *state = static_cast<ListDevicesState *>(state_voidptr);
  if (eol || !info) {
    state->finished = true;
    return;
  }
  EngineDevice device;
  device.value = info->name ? info->name : "";
  device.description = info->description ? info->description : device.value;
  device.iconname = device.GuessIconName();
  state->devices.push_back(device);
}
#endif

EngineDeviceList PulseDeviceFinder::ListDevices() {
#ifdef HAVE_PULSE
  if (!context_ || pa_context_get_state(context_) != PA_CONTEXT_READY) {
    return {};
  }
  ListDevicesState state;
  pa_context_get_sink_info_list(context_, &PulseDeviceFinder::GetSinkInfoCallback, &state);
  for (int i = 0; i < 200 && !state.finished; ++i) {
    pa_mainloop_iterate(mainloop_, 0, nullptr);
    g_usleep(10000);
  }
  return state.devices;
#else
  return {};
#endif
}
