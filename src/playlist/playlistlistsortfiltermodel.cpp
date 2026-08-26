#include "playlist/playlistlistsortfiltermodel.h"

#include "utilities/strutils.h"

PlaylistListSortFilterModel::PlaylistListSortFilterModel(PlaylistListModel *source) : source_(source) {}

void PlaylistListSortFilterModel::SetFilter(const std::string &filter) { filter_ = filter; }

void PlaylistListSortFilterModel::SetFavoritesOnly(bool favorites_only) { favorites_only_ = favorites_only; }

std::vector<std::string> PlaylistListSortFilterModel::Visible() const {
  std::vector<std::string> names;
  if (!source_) {
    return names;
  }
  for (int i = 0; i < source_->Count(); ++i) {
    if (favorites_only_ && !source_->favorites()[static_cast<size_t>(i)]) {
      continue;
    }
    if (!filter_.empty() && !StrUtils::ContainsInsensitive(source_->At(i), filter_)) {
      continue;
    }
    names.push_back(source_->At(i));
  }
  std::sort(names.begin(), names.end());
  return names;
}
