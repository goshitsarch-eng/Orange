#ifndef STRAWBERRY_FILEVIEWTREEMODEL_H
#define STRAWBERRY_FILEVIEWTREEMODEL_H

#include "fileview/fileviewtreeitem.h"

#include <memory>
#include <string>
#include <vector>

class FileViewTreeModel {
 public:
  void SetRootPaths(const std::vector<std::string> &paths);
  void SetNameFilters(const std::vector<std::string> &filters);
  void LazyLoad(FileViewTreeItem *item);
  FileViewTreeItem *root() { return root_.get(); }
  const FileViewTreeItem *root() const { return root_.get(); }
  int DirectoryCount() const;
  std::vector<std::string> FilesIn(const std::string &directory) const;
  const std::vector<std::string> &name_filters() const { return name_filters_; }

 private:
  void Reset();
  bool AcceptsFile(const std::string &path) const;

  std::unique_ptr<FileViewTreeItem> root_;
  std::vector<std::string> name_filters_;
};

#endif
