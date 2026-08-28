#ifndef STRAWBERRY_TRAYMENUPLAYPAUSE_H
#define STRAWBERRY_TRAYMENUPLAYPAUSE_H

namespace TrayMenuPlayPause {

// Qt SystemTrayIcon::SetPlaying(enable_play_pause) greys out Play/Pause for PauseDisabled streams.
inline bool ItemEnabled(int id, int play_pause_id, bool enabled) { return id != play_pause_id || enabled; }

}  // namespace TrayMenuPlayPause

#endif
