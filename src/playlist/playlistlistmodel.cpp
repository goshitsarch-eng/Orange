#include "playlist/playlistlistmodel.h"

void PlaylistListModel::Reload(PlaylistManager *manager) {
  names_.clear();
  favorites_.clear();
  paths_.clear();
  if (!manager) {
    return;
  }
  for (const auto &playlist : manager->playlists()) {
    names_.push_back(playlist->name());
    favorites_.push_back(playlist->favorite());
    paths_.push_back(playlist->ui_path());
  }
}

void PlaylistListModel::SetRows(const std::vector<std::string> &names, const std::vector<bool> &favorites, const std::vector<std::string> &paths) {
  names_ = names;
  favorites_ = favorites;
  paths_ = paths;
  if (favorites_.size() < names_.size()) {
    favorites_.resize(names_.size(), false);
  }
  if (paths_.size() < names_.size()) {
    paths_.resize(names_.size());
  }
}

const std::string &PlaylistListModel::At(int index) const {
  static const std::string empty;
  if (index < 0 || index >= Count()) {
    return empty;
  }
  return names_[static_cast<size_t>(index)];
}

int PlaylistListModel::IndexOf(const std::string &name) const {
  for (int i = 0; i < Count(); ++i) {
    if (names_[static_cast<size_t>(i)] == name) {
      return i;
    }
  }
  return -1;
}

std::string PlaylistListModel::PathOf(const std::string &name) const {
  const int index = IndexOf(name);
  if (index < 0 || index >= static_cast<int>(paths_.size())) {
    return {};
  }
  return paths_[static_cast<size_t>(index)];
}
