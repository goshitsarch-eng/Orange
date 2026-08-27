#ifndef STRAWBERRY_DELETEFILESPOLICY_H
#define STRAWBERRY_DELETEFILESPOLICY_H

#include "constants/collectionsettings.h"
#include "constants/playlistsettings.h"
#include "core/settings.h"
#include "core/song.h"

namespace DeleteFilesPolicy {

enum class Source { Collection, Playlist };

inline bool Allowed(Source source, bool collection_enabled, bool playlist_enabled) {
  return source == Source::Collection ? collection_enabled : playlist_enabled;
}

inline Source SourceForSongs(const SongList &songs) {
  if (songs.empty()) {
    return Source::Playlist;
  }
  for (const Song &song : songs) {
    if (song.source() != Song::Source::Collection) {
      return Source::Playlist;
    }
  }
  return Source::Collection;
}

inline bool Allowed(Source source) {
  Settings settings;
  if (source == Source::Collection) {
    settings.BeginGroup(CollectionSettings::kSettingsGroup);
    return settings.BoolValue(CollectionSettings::kDeleteFiles, CollectionSettings::kDefaultDeleteFiles);
  }
  settings.BeginGroup(PlaylistSettings::kSettingsGroup);
  return settings.BoolValue(PlaylistSettings::kDeleteFiles, PlaylistSettings::kDefaultDeleteFiles);
}

inline const char *DeniedMessage(Source source) {
  return source == Source::Collection ? "Deleting files from the collection is disabled in Preferences."
                                      : "Deleting files from the playlist is disabled in Preferences.";
}

}  // namespace DeleteFilesPolicy

#endif
