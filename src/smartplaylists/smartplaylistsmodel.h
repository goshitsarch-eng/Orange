#ifndef STRAWBERRY_SMARTPLAYLISTSMODEL_H
#define STRAWBERRY_SMARTPLAYLISTSMODEL_H

#include "smartplaylists/smartplaylistsitem.h"

#include <string>
#include <vector>

class SmartPlaylistsModel {
 public:
  void Reload();
  void RestoreDefaults();
  const std::vector<SmartPlaylistsItem> &items() const { return items_; }
  const SmartPlaylistsItem *ItemByKey(const std::string &key) const;
  int row_count() const { return static_cast<int>(items_.size()); }

  static std::vector<SmartPlaylistsItem> BuiltinItems();

 private:
  std::vector<SmartPlaylistsItem> items_;
};

#endif
