#ifndef STRAWBERRY_SUBSONICSETTINGSACTIONS_H
#define STRAWBERRY_SUBSONICSETTINGSACTIONS_H

#include "collection/collectionbackend.h"
#include "core/song.h"

namespace SubsonicSettingsActions {

inline const char *DeleteSongs() { return "Delete songs"; }
inline const char *DeleteSongsTitle() { return "Delete songs"; }
inline const char *DeleteSongsBody() { return "Delete all cached Subsonic songs from the collection?"; }

inline int DeleteCachedSongs(CollectionBackend *backend) {
  return backend ? backend->DeleteSongsBySource(Song::Source::Subsonic) : 0;
}

}  // namespace SubsonicSettingsActions

#endif
