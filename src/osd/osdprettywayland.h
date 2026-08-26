#ifndef STRAWBERRY_OSDPRETTYWAYLAND_H
#define STRAWBERRY_OSDPRETTYWAYLAND_H

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

}  // namespace OSDPrettyWayland

#endif

