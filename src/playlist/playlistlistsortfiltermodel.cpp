#include "playlist/playlistlistsortfiltermodel.h"

#include "utilities/strutils.h"

PlaylistListSortFilterModel::PlaylistListSortFilterModel(PlaylistListModel *source) : source_(source) {}

void PlaylistListSortFilterModel::SetFilter(const std::string &filter) { filter_ = filter; }

void PlaylistListSortFilterModel::SetFavoritesOnly(bool favorites_only) { favorites_only_ = favorites_only; }

std::vector<PlaylistListDrop::Row> PlaylistListSortFilterModel::VisibleRows() const {
  std::vector<PlaylistListDrop::Row> rows;
  if (!source_) {
    return rows;
  }
  for (int i = 0; i < source_->Count(); ++i) {
    const bool favorite = i < static_cast<int>(source_->favorites().size()) && source_->favorites()[static_cast<size_t>(i)];
    if (favorites_only_ && !favorite) {
      continue;
    }
    if (!filter_.empty() && !StrUtils::ContainsInsensitive(source_->At(i), filter_)) {
      continue;
    }
    rows.push_back({source_->At(i), favorite});
  }
  std::sort(rows.begin(), rows.end(), [](const PlaylistListDrop::Row &a, const PlaylistListDrop::Row &b) { return a.name < b.name; });
  return rows;
}

std::vector<std::string> PlaylistListSortFilterModel::Visible() const {
  std::vector<std::string> names;
  for (const PlaylistListDrop::Row &row : VisibleRows()) {
    names.push_back(row.name);
  }
  return names;
}
