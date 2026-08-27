#ifndef STRAWBERRY_SMARTPLAYLISTTAGCOMPLETER_H
#define STRAWBERRY_SMARTPLAYLISTTAGCOMPLETER_H

#include "playlist/playlisttagcompletion.h"
#include "smartplaylists/smartplaylist.h"
#include "smartplaylists/smartplaylisttermvalue.h"

namespace SmartPlaylistTagCompleter {

// Qt SmartPlaylistSearchTermWidget::FieldChanged attaches TagCompleter for artist/album fields.
inline bool CompletesField(SmartPlaylistField field) {
  switch (field) {
    case SmartPlaylistField::Artist:
    case SmartPlaylistField::Album:
    case SmartPlaylistField::AlbumArtist:
      return true;
    default:
      return false;
  }
}

inline PlaylistColumn ColumnFor(SmartPlaylistField field) {
  switch (field) {
    case SmartPlaylistField::Album:
      return PlaylistColumn::Album;
    case SmartPlaylistField::AlbumArtist:
      return PlaylistColumn::AlbumArtist;
    case SmartPlaylistField::Artist:
    default:
      return PlaylistColumn::Artist;
  }
}

inline std::vector<std::string> ValuesFor(const SongList &songs, SmartPlaylistField field) {
  if (!CompletesField(field)) {
    return {};
  }
  return PlaylistTagCompletion::UniqueValues(songs, ColumnFor(field));
}

inline bool ShouldAttach(SmartPlaylistTermValue::Editor editor, SmartPlaylistField field) {
  return editor == SmartPlaylistTermValue::Editor::Text && CompletesField(field);
}

}  // namespace SmartPlaylistTagCompleter

#endif
