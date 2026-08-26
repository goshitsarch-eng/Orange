#ifndef STRAWBERRY_PLAYLISTLISTSORTFILTERMODEL_H
#define STRAWBERRY_PLAYLISTLISTSORTFILTERMODEL_H

#include "playlist/playlistlistdrop.h"
#include "playlist/playlistlistmodel.h"

#include <algorithm>
#include <string>
#include <vector>

class PlaylistListSortFilterModel {
 public:
  explicit PlaylistListSortFilterModel(PlaylistListModel *source = nullptr);

  void SetSource(PlaylistListModel *source) { source_ = source; }
  void SetFilter(const std::string &filter);
  void SetFavoritesOnly(bool favorites_only);
  const std::string &filter() const { return filter_; }
  bool favorites_only() const { return favorites_only_; }
  std::vector<PlaylistListDrop::Row> VisibleRows() const;
  std::vector<std::string> Visible() const;

 private:
  PlaylistListModel *source_ = nullptr;
  std::string filter_;
  bool favorites_only_ = false;
};

#endif
