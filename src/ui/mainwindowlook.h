#ifndef STRAWBERRY_MAINWINDOWLOOK_H
#define STRAWBERRY_MAINWINDOWLOOK_H

#include "constants/backendsettings.h"
#include "core/mainwindowsettings.h"

namespace MainWindowLook {

inline bool ShowSidebar(bool stored) { return stored; }

inline bool DefaultShowSidebar() { return MainWindowSettings::kDefaultShowSidebar; }

inline bool MuteVisible(bool volume_control) { return volume_control; }

inline bool MuteVisibleFromSettings(bool volume_control_stored, bool default_volume_control = BackendSettings::kDefaultVolumeControl) {
  (void)default_volume_control;
  return MuteVisible(volume_control_stored);
}

inline bool IsMuted(unsigned volume) { return volume == 0; }

inline const char *MuteIconName(bool muted) { return muted ? "audio-volume-muted-symbolic" : "audio-volume-high-symbolic"; }

inline const char *MuteTooltip(bool muted) { return muted ? "Unmute" : "Mute"; }

inline const char *MuteAccel() { return "<Control>m"; }

inline const char *ClosePlaylistAccel() { return "<Control>w"; }

inline const char *PlaylistQueueAccel() { return "<Control>d"; }

inline const char *QueuePlayNextAccel() { return "<Control><Shift>d"; }

}  // namespace MainWindowLook

#endif
