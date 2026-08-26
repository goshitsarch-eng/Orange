#ifndef STRAWBERRY_PLAYLISTSETTINGSLABELS_H
#define STRAWBERRY_PLAYLISTSETTINGSLABELS_H

namespace PlaylistSettingsLabels {

inline const char *Alternating() { return "Use alternating row colors"; }
inline const char *Bars() { return "Show bars on the currently playing track"; }
inline const char *Glow() { return "Show a glowing animation on the currently playing track"; }
inline const char *WarnClose() { return "Warn me when closing a playlist tab"; }
inline const char *ContinueOnError() { return "Continue to the next item in the playlist if a song is unavailable"; }
inline const char *GreyoutPlay() { return "Grey out unavailable songs in playlists on playback"; }
inline const char *GreyoutStartup() { return "Grey out unavailable songs in playlists on startup"; }
inline const char *SelectTrack() { return "Automatically select current playing track"; }
inline const char *Toolbar() { return "Enable playlist toolbar"; }
inline const char *Clear() { return "Enable playlist clear button"; }
inline const char *DeleteFiles() { return "Enable delete files in the right click context menu"; }
inline const char *AutoSort() { return "Automatically sort playlist when inserting songs"; }
inline const char *PathsGroup() { return "When saving a playlist, file paths should be"; }
inline const char *InlineEdit() { return "Enable song metadata inline edition with click"; }
inline const char *WriteMetadata() { return "Write metadata when saving playlists"; }

}  // namespace PlaylistSettingsLabels

#endif
