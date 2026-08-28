#ifndef STRAWBERRY_SMARTPLAYLISTSSHOW_H
#define STRAWBERRY_SMARTPLAYLISTSSHOW_H

namespace SmartPlaylistsShow {

// Qt SmartPlaylistsViewContainer::showEvent calls ItemsSelectedChanged to refresh edit/delete.
inline bool ShouldRefreshSelectionOnShow() { return true; }

}  // namespace SmartPlaylistsShow

#endif
