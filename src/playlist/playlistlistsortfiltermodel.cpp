#include "playlist/playlistlistsortfiltermodel.h"

#include "playlist/playlistfolders.h"

PlaylistListSortFilterModel::PlaylistListSortFilterModel(PlaylistListModel *source) : source_(source) {}

void PlaylistListSortFilterModel::SetFilter(const std::string &filter) { filter_ = filter; }

void PlaylistListSortFilterModel::SetFavoritesOnly(bool favorites_only) { favorites_only_ = favorites_only; }

std::vector<PlaylistListDrop::Row> PlaylistListSortFilterModel::VisibleRows() const {
  std::vector<PlaylistFolders::PlaylistRef> playlists;
  if (!source_) {
    return {};
  }
  for (int i = 0; i < source_->Count(); ++i) {
    PlaylistFolders::PlaylistRef playlist;
    playlist.name = source_->At(i);
    playlist.favorite = i < static_cast<int>(source_->favorites().size()) && source_->favorites()[static_cast<size_t>(i)];
    if (i < static_cast<int>(source_->paths().size())) {
      playlist.ui_path = source_->paths()[static_cast<size_t>(i)];
    }
    playlists.push_back(playlist);
  }
  return PlaylistFolders::Flatten(playlists, extra_folders_, collapsed_, filter_, favorites_only_);
}

std::vector<std::string> PlaylistListSortFilterModel::Visible() const {
  std::vector<std::string> names;
  for (const PlaylistListDrop::Row &row : VisibleRows()) {
    if (!row.folder) {
      names.push_back(row.name);
    }
  }
  return names;
}
