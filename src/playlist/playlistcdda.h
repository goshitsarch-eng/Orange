#ifndef STRAWBERRY_PLAYLISTCDDA_H
#define STRAWBERRY_PLAYLISTCDDA_H

#include "collection/collectionbehaviour.h"
#include "constants/behavioursettings.h"

#include <string>
#include <vector>

namespace PlaylistCdda {

// Qt Playlist::kCddaMimeType.
inline const char *kMimeType() { return "x-content/audio-cdda"; }

// Qt MimeData::get_name_for_new_playlist when name_for_new_playlist_ is empty.
inline const char *NewPlaylistName() { return "Playlist"; }

// Qt MainWindow::AddCDTracks sets open_in_new_playlist_.
inline bool OpensInNewPlaylist() { return true; }

inline bool HasMime(const std::string &mime) { return mime == kMimeType(); }

// Qt AddToPlaylist applies menu play after opening the new playlist.
inline CollectionBehaviour::Plan MenuPlan(BehaviourSettings::PlayBehaviour play, bool engine_stopped) {
  return CollectionBehaviour::OpenInNew(play, engine_stopped);
}

inline const char *EmptyError() { return "No audio CD found"; }
inline const char *FallbackError() { return "Error while loading audio CD."; }
inline const char *MissingPlaybackError() { return "Missing CDDA playback."; }

inline std::string ErrorOrFallback(const std::string &error) { return error.empty() ? FallbackError() : error; }

inline bool ShouldRefreshView(const void *changed, const void *current) { return changed != nullptr && changed == current; }

inline void AppendCddaMount(std::vector<std::string> *out, const std::string &backend, const std::string &mount_path) {
  if (!out || backend != "cdda" || mount_path.empty()) {
    return;
  }
  out->push_back(mount_path);
}

}  // namespace PlaylistCdda

#endif
