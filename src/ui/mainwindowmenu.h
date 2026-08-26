#ifndef STRAWBERRY_MAINWINDOWMENU_H
#define STRAWBERRY_MAINWINDOWMENU_H

namespace MainWindowMenu {

inline const char *UpdateCollection() { return "Update changed collection folders"; }
inline const char *EditTrack() { return "Edit track information..."; }
inline const char *RenumberTracks() { return "Renumber tracks in this order..."; }
inline const char *SetValue() { return "Set value for all selected tracks..."; }
inline const char *Love() { return "Love"; }
inline const char *ToggleScrobbling() { return "Toggle scrobbling"; }
inline const char *ShuffleMode() { return "Shuffle mode"; }
inline const char *RepeatMode() { return "Repeat mode"; }
inline const char *ToggleScrobblingAction() { return "win.toggle-scrobbling"; }
inline const char *ShuffleModeAction() { return "win.shuffle-mode"; }
inline const char *RepeatModeAction() { return "win.repeat-mode"; }

}  // namespace MainWindowMenu

#endif
