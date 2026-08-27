#ifndef STRAWBERRY_MAINWINDOWMENU_H
#define STRAWBERRY_MAINWINDOWMENU_H

namespace MainWindowMenu {

inline const char *OpenFile() { return "Open file..."; }
inline const char *AddFile() { return "Add file..."; }
inline const char *AddFolder() { return "Add folder..."; }
inline const char *OpenCD() { return "Open audio CD..."; }
inline const char *AddStream() { return "Add stream..."; }
inline const char *UpdateCollection() { return "Update changed collection folders"; }
inline const char *FullScan() { return "Do a full collection rescan"; }
inline const char *StopScan() { return "Stop collection scan"; }
inline const char *Mute() { return "Mute"; }
inline const char *NewPlaylist() { return "New playlist"; }
inline const char *LoadPlaylist() { return "Load playlist..."; }
inline const char *SavePlaylist() { return "Save playlist..."; }
inline const char *SaveAllPlaylists() { return "Save all playlists..."; }
inline const char *ClosePlaylist() { return "Close current playlist tab"; }
inline const char *ClearPlaylist() { return "Clear playlist"; }
inline const char *ShufflePlaylist() { return "Shuffle playlist"; }
inline const char *RemoveDuplicates() { return "Remove duplicates from playlist"; }
inline const char *RemoveUnavailable() { return "Remove unavailable tracks from playlist"; }
inline const char *RenumberTracks() { return "Renumber tracks in this order..."; }
inline const char *JumpToPlaying() { return "Jump to the currently playing track"; }
inline const char *GoNextTab() { return "Go to next playlist tab"; }
inline const char *GoPreviousTab() { return "Go to previous playlist tab"; }
inline const char *GoLastTab() { return "Go to last playlist tab"; }
inline const char *GoActiveTab() { return "Go to active playlist tab"; }
inline const char *EditTrack() { return "Edit track information..."; }
inline const char *CompleteTags() { return "Complete tags automatically..."; }
inline const char *SetValue() { return "Set value for all selected tracks..."; }
inline const char *Play() { return "Play"; }
inline const char *Stop() { return "Stop"; }
inline const char *PreviousTrack() { return "Previous track"; }
inline const char *NextTrack() { return "Next track"; }
inline const char *StopAfter() { return "Stop after this track"; }
inline const char *Love() { return "Love"; }
inline const char *ToggleScrobbling() { return "Toggle scrobbling"; }
inline const char *ShuffleMode() { return "Shuffle mode"; }
inline const char *RepeatMode() { return "Repeat mode"; }
inline const char *CoverManager() { return "Cover Manager"; }
inline const char *Equalizer() { return "Equalizer"; }
inline const char *Transcode() { return "Transcode Music"; }
inline const char *Console() { return "Console"; }
inline const char *ShowSidebar() { return "Show sidebar"; }
inline const char *Settings() { return "Settings..."; }
inline const char *About() { return "About Strawberry"; }
inline const char *Quit() { return "Quit"; }
inline const char *ToggleScrobblingAction() { return "win.toggle-scrobbling"; }
inline const char *ShuffleModeAction() { return "win.shuffle-mode"; }
inline const char *RepeatModeAction() { return "win.repeat-mode"; }

}  // namespace MainWindowMenu

#endif
