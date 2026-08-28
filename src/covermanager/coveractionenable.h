#ifndef STRAWBERRY_COVERACTIONENABLE_H
#define STRAWBERRY_COVERACTIONENABLE_H

#include "core/song.h"
#include "covermanager/coverchoicemenu.h"

namespace CoverActionEnable {

// Qt MainWindow::SongChanged: only local collection albums can change art.
inline bool CanChangeArt(const Song &song) {
  return song.is_collection_song() && !song.EffectiveAlbumartist().empty() && !song.album().empty();
}

// Qt Song::has_valid_art && !art_unset.
inline bool HasValidArt(const Song &song) {
  return !song.art_unset() && (song.art_embedded() || !song.art_automatic().empty() || !song.art_manual().empty());
}

inline bool Enabled(CoverChoiceMenu::Action action, const Song &song, bool has_providers = true) {
  switch (action) {
    case CoverChoiceMenu::Action::Show:
    case CoverChoiceMenu::Action::Save:
      return HasValidArt(song);
    case CoverChoiceMenu::Action::File:
    case CoverChoiceMenu::Action::Url:
    case CoverChoiceMenu::Action::Fetch:
      return CanChangeArt(song);
    case CoverChoiceMenu::Action::Search:
      return has_providers && CanChangeArt(song);
    case CoverChoiceMenu::Action::Unset:
      return CanChangeArt(song) && !song.art_unset();
    case CoverChoiceMenu::Action::Clear:
      return CanChangeArt(song) && !song.art_manual().empty();
    case CoverChoiceMenu::Action::Delete:
      return CanChangeArt(song) && (song.art_embedded() || !song.art_automatic().empty() || !song.art_manual().empty());
  }
  return false;
}

}  // namespace CoverActionEnable

#endif
