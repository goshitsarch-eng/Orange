#ifndef STRAWBERRY_PLAYLISTTOOLBAR_H
#define STRAWBERRY_PLAYLISTTOOLBAR_H

#include "constants/appearancesettings.h"
#include "core/appearancecolors.h"

namespace PlaylistToolbar {

// Qt PlaylistContainer::ReloadSettings / MaybeUpdateFilter / FocusSearchField.
inline bool Visible(bool show_toolbar) { return show_toolbar; }

inline bool ShouldClearFilter(bool show_toolbar) { return !show_toolbar; }

inline bool FilterApplies(bool show_toolbar) { return show_toolbar; }

inline bool FocusEnabled(bool show_toolbar) { return show_toolbar; }

// Qt PlaylistContainer::ReloadSettings applies AppearanceSettings::kIconSizePlaylistButtons.
inline const char *CssClass() { return "strawberry-playlist-buttons"; }

inline bool ShouldApplyIconSizes(bool toolbar_visible) { return toolbar_visible; }

inline int IconSize(int stored) {
  return AppearanceColors::ClampIcon(stored, AppearanceSettings::kDefaultIconSizePlaylistButtons);
}

}  // namespace PlaylistToolbar

#endif
