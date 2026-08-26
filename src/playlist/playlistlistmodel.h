#ifndef STRAWBERRY_PLAYLISTLISTMODEL_H
#define STRAWBERRY_PLAYLISTLISTMODEL_H

#include "playlist/playlistmanager.h"

#include <string>
#include <vector>

class PlaylistListModel {
 public:
  void Reload(PlaylistManager *manager);
  void SetRows(const std::vector<std::string> &names, const std::vector<bool> &favorites, const std::vector<std::string> &paths = {});
  int Count() const { return static_cast<int>(names_.size()); }
  const std::string &At(int index) const;
  int IndexOf(const std::string &name) const;
  const std::vector<std::string> &names() const { return names_; }
  const std::vector<bool> &favorites() const { return favorites_; }
  const std::vector<std::string> &paths() const { return paths_; }
  std::string PathOf(const std::string &name) const;

 private:
  std::vector<std::string> names_;
  std::vector<bool> favorites_;
  std::vector<std::string> paths_;
};

#endif
