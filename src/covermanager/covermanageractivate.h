#ifndef COVERMANAGER_COVERMANAGERACTIVATE_H_
#define COVERMANAGER_COVERMANAGERACTIVATE_H_

#include "widgets/listboxkeyboard.h"

// Qt AlbumCoverManager connects albums doubleClicked → ShowCover only.
// QListWidget Enter emits activated (unconnected) and never adds to the playlist —
// add/load are context-menu and toolbar actions. GTK keyboard activate uses the
// same ShowCover path as double-click so Enter is not a silent no-op.

struct CoverManagerActivate {
  enum class Action {
    None,
    ShowCover,
    AddToPlaylist,
  };

  static bool IsEnter(unsigned keyval) {
    return keyval == ListBoxKeyboard::kReturn || keyval == ListBoxKeyboard::kKPEnter;
  }

  static Action ForAlbumEnter() { return Action::ShowCover; }

  static Action ForArtistEnter() { return Action::None; }

  static bool AlbumEnterAddsToPlaylist() { return ForAlbumEnter() == Action::AddToPlaylist; }
};

#endif  // COVERMANAGER_COVERMANAGERACTIVATE_H_
