#ifndef STRAWBERRY_OSDPRETTYWAYLAND_H
#define STRAWBERRY_OSDPRETTYWAYLAND_H

#include "config.h"

namespace OSDPrettyWayland {

enum class PositionBackend { None, X11, LayerShell, Unpositioned };

inline bool SupportedOnDisplay(bool has_display) { return has_display; }

inline PositionBackend DetectBackend(bool is_x11_display, bool layer_shell_available) {
  if (is_x11_display) {
    return PositionBackend::X11;
  }
  if (layer_shell_available) {
    return PositionBackend::LayerShell;
  }
  return PositionBackend::Unpositioned;
}

inline bool CanMoveWindow(PositionBackend backend) {
  return backend == PositionBackend::X11 || backend == PositionBackend::LayerShell;
}

inline bool CompiledWithLayerShell() {
#ifdef HAVE_GTK4_LAYER_SHELL
  return true;
#else
  return false;
#endif
}

inline bool RuntimeLayerShell(bool compiled, bool compositor_supported) { return compiled && compositor_supported; }

struct LayerMargins {
  int left = 0;
  int top = 0;
};

inline LayerMargins MarginsFromAbsolute(int abs_x, int abs_y, int workarea_x, int workarea_y) {
  return {abs_x - workarea_x, abs_y - workarea_y};
}

}  // namespace OSDPrettyWayland

#endif

