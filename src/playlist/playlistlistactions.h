#ifndef STRAWBERRY_PLAYLISTLISTACTIONS_H
#define STRAWBERRY_PLAYLISTLISTACTIONS_H

namespace PlaylistListActions {

// Qt PlaylistListContainer::ItemsSelectedChanged / showEvent.
inline bool SelectionActionsEnabled(bool items_selected) { return items_selected; }

inline bool RemoveEnabled(bool items_selected) { return items_selected; }

inline bool SaveEnabled(bool items_selected) { return items_selected; }

inline bool CopyEnabled(bool items_selected) { return items_selected; }

inline bool ShouldRefreshOnShow() { return true; }

}  // namespace PlaylistListActions

#endif
