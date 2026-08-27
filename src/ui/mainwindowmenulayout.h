#ifndef STRAWBERRY_MAINWINDOWMENULAYOUT_H
#define STRAWBERRY_MAINWINDOWMENULAYOUT_H

#include <cstring>

namespace MainWindowMenuLayout {

enum class Section { Music, Playlist };

// Qt menu_music: Open file, Open CD. Qt menu_playlist starts with Add file/folder/stream.
inline Section OpenFile() { return Section::Music; }
inline Section OpenCD() { return Section::Music; }
inline Section AddFile() { return Section::Playlist; }
inline Section AddFolder() { return Section::Playlist; }
inline Section AddStream() { return Section::Playlist; }

inline bool IsPlaylistAddAction(const char *action) {
  return action && (std::strcmp(action, "add-file") == 0 || std::strcmp(action, "add-folder") == 0 || std::strcmp(action, "add-stream") == 0);
}

}  // namespace MainWindowMenuLayout

#endif
