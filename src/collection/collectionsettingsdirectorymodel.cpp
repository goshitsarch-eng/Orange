#include "collection/collectionsettingsdirectorymodel.h"

#include <algorithm>

void CollectionSettingsDirectoryModel::Add(const std::string &path) {
  if (path.empty()) {
    return;
  }
  if (std::find(paths_.begin(), paths_.end(), path) == paths_.end()) {
    paths_.push_back(path);
  }
}

void CollectionSettingsDirectoryModel::Remove(const std::string &path) {
  paths_.erase(std::remove(paths_.begin(), paths_.end(), path), paths_.end());
}
