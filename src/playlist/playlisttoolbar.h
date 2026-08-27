#ifndef STRAWBERRY_PLAYLISTTOOLBAR_H
#define STRAWBERRY_PLAYLISTTOOLBAR_H

namespace PlaylistToolbar {

// Qt PlaylistContainer::ReloadSettings / MaybeUpdateFilter / FocusSearchField.
inline bool Visible(bool show_toolbar) { return show_toolbar; }

inline bool ShouldClearFilter(bool show_toolbar) { return !show_toolbar; }

inline bool FilterApplies(bool show_toolbar) { return show_toolbar; }

inline bool FocusEnabled(bool show_toolbar) { return show_toolbar; }

}  // namespace PlaylistToolbar

#endif
