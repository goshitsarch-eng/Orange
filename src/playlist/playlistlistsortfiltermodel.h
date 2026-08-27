#ifndef STRAWBERRY_PLAYLISTLISTSORTFILTERMODEL_H
#define STRAWBERRY_PLAYLISTLISTSORTFILTERMODEL_H

#include "playlist/playlistlistdrop.h"
#include "playlist/playlistlistmodel.h"

#include <set>
#include <string>
#include <vector>

class PlaylistListSortFilterModel {
 public:
  explicit PlaylistListSortFilterModel(PlaylistListModel *source = nullptr);

  void SetSource(PlaylistListModel *source) { source_ = source; }
  void SetFilter(const std::string &filter);
  void SetFavoritesOnly(bool favorites_only);
  void SetExtraFolders(const std::vector<std::string> &folders) { extra_folders_ = folders; }
  void SetCollapsed(const std::set<std::string> &collapsed) { collapsed_ = collapsed; }
  const std::string &filter() const { return filter_; }
  bool favorites_only() const { return favorites_only_; }
  const std::vector<std::string> &extra_folders() const { return extra_folders_; }
  const std::set<std::string> &collapsed() const { return collapsed_; }
  std::vector<PlaylistListDrop::Row> VisibleRows() const;
  std::vector<std::string> Visible() const;

 private:
  PlaylistListModel *source_ = nullptr;
  std::string filter_;
  bool favorites_only_ = false;
  std::vector<std::string> extra_folders_;
  std::set<std::string> collapsed_;
};

#endif
