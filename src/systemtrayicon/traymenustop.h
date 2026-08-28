#ifndef STRAWBERRY_TRAYMENUSTOP_H
#define STRAWBERRY_TRAYMENUSTOP_H

namespace TrayMenuStop {

// Qt SystemTrayIcon::SetPlaying/SetPaused enable Stop and Stop after; SetStopped disables them.
inline bool PlaybackActive(bool playing, bool paused) { return playing || paused; }

inline bool IsStopId(int id, int stop_id, int stop_after_id) { return id == stop_id || id == stop_after_id; }

inline bool ItemEnabled(int id, int stop_id, int stop_after_id, bool active) {
  return !IsStopId(id, stop_id, stop_after_id) || active;
}

}  // namespace TrayMenuStop

#endif
