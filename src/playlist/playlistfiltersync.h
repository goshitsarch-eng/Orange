#ifndef STRAWBERRY_PLAYLISTFILTERSYNC_H
#define STRAWBERRY_PLAYLISTFILTERSYNC_H

#include "playlist/playlist.h"

#include <string>

namespace PlaylistFilterSync {

inline std::string FilterForPlaylist(const Playlist *playlist) { return playlist ? playlist->filter_string() : std::string(); }

inline std::string EntryFromPlaylist(const std::string &stored) { return stored; }

inline bool ShouldSyncEntry(const std::string &entry, const std::string &stored) { return entry != stored; }

}  // namespace PlaylistFilterSync

#endif  // STRAWBERRY_PLAYLISTFILTERSYNC_H
