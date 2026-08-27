#ifndef STRAWBERRY_PULSEDEVICEFINDER_H
#define STRAWBERRY_PULSEDEVICEFINDER_H

#include "config.h"
#include "engine/devicefinder.h"

#ifdef HAVE_PULSE
#include <pulse/context.h>
#include <pulse/introspect.h>
#include <pulse/mainloop.h>
#endif

class PulseDeviceFinder : public DeviceFinder {
 public:
  PulseDeviceFinder();
  ~PulseDeviceFinder() override;

  bool Initialize() override;
  EngineDeviceList ListDevices() override;

 private:
#ifdef HAVE_PULSE
  struct ListDevicesState {
    bool finished = false;
    EngineDeviceList devices;
  };

  bool Reconnect();
  static void GetSinkInfoCallback(pa_context *c, const pa_sink_info *info, int eol, void *state);

  pa_mainloop *mainloop_ = nullptr;
  pa_context *context_ = nullptr;
#endif
};

#endif
