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
  void SetShowHidden(bool show_hidden);
  void SetShowAllFiles(bool show_all);
  void LazyLoad(FileViewTreeItem *item);
  FileViewTreeItem *root() { return root_.get(); }
  const FileViewTreeItem *root() const { return root_.get(); }
  int DirectoryCount() const;
  std::vector<std::string> FilesIn(const std::string &directory) const;
  const std::vector<std::string> &name_filters() const { return name_filters_; }
  bool show_hidden() const { return show_hidden_; }
  bool show_all_files() const { return show_all_files_; }

 private:
  void Reset();
  bool AcceptsFile(const std::string &path) const;

  std::unique_ptr<FileViewTreeItem> root_;
  std::vector<std::string> name_filters_;
  bool show_hidden_ = false;
  bool show_all_files_ = false;
};

#endif
