#ifndef STRAWBERRY_PLAYLISTLISTLEFT_H
#define STRAWBERRY_PLAYLISTLISTLEFT_H

#include "collection/collectiontreeleft.h"
#include "playlist/playlistfolders.h"

#include <string>

namespace PlaylistListLeft {

using Action = CollectionTreeLeft::Action;

// Qt AutoExpandingTreeView Left: playlists are leaves, folders are containers.
inline bool IsRootRow(bool folder, const std::string &path) {
  return folder ? PlaylistFolders::Parent(path).empty() : path.empty();
}

inline std::string ParentPath(bool folder, const std::string &path) {
  return folder ? PlaylistFolders::Parent(path) : path;
}

inline Action FromRow(bool folder, bool expanded, const std::string &path) {
  return CollectionTreeLeft::FromState(IsRootRow(folder, path), folder && expanded, folder);
}

inline std::string FocusPath(bool folder, bool expanded, const std::string &path) {
  const Action action = FromRow(folder, expanded, path);
  if (action == Action::SelectParentAndCollapse) {
    return ParentPath(folder, path);
  }
  if (action == Action::CollapseCurrent) {
    return path;
  }
  return {};
}

// A visible child means the parent folder is expanded, so Left collapses that parent.
inline std::string CollapsePath(bool folder, bool expanded, const std::string &path) {
  const Action action = FromRow(folder, expanded, path);
  if (action == Action::CollapseCurrent) {
    return path;
  }
  if (action == Action::SelectParentAndCollapse) {
    return ParentPath(folder, path);
  }
  return {};
}

}  // namespace PlaylistListLeft

#endif
