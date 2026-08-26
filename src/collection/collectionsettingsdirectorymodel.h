#ifndef STRAWBERRY_COLLECTIONSETTINGSDIRECTORYMODEL_H
#define STRAWBERRY_COLLECTIONSETTINGSDIRECTORYMODEL_H

#include <string>
#include <vector>

class CollectionSettingsDirectoryModel {
 public:
  void SetPaths(const std::vector<std::string> &paths) { paths_ = paths; }
  const std::vector<std::string> &paths() const { return paths_; }
  void Add(const std::string &path);
  void Remove(const std::string &path);

 private:
  std::vector<std::string> paths_;
};

#endif
