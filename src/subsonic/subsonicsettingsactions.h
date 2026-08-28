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

// Qt SubsonicSettingsPage::eventFilter re-enables the Test button on QEvent::Enter (mouse enter).
inline bool ShouldReenableTestOnEnter() { return true; }

// Map/show is the GTK equivalent of entering the dialog after a Test click disables the button.
inline bool ShouldReenableTestOnShow() { return true; }

}  // namespace SubsonicSettingsActions

#endif
