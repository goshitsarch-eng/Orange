#ifndef STRAWBERRY_FILEVIEW_H
#define STRAWBERRY_FILEVIEW_H

#include "fileview/fileviewlist.h"
#include "fileview/fileviewtree.h"
#include "fileview/fileviewtreemodel.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class FileView {
 public:
  using PathsCallback = std::function<void(const std::vector<std::string> &)>;

  FileView();

  GtkWidget *widget() const { return widget_; }
  FileViewTreeModel *model() { return &model_; }
  FileViewList *list() { return list_.get(); }
  FileViewTree *tree() { return tree_.get(); }
  const std::string &path() const { return path_; }

  void SetPath(const std::string &path);
  void FileUp();
  void FileHome();
  void Reload();
  void SetAddToPlaylistCallback(PathsCallback callback);
  void SetCopyToCollectionCallback(PathsCallback callback);
  void SetCopyToDeviceCallback(PathsCallback callback);
  void SetEditTagsCallback(PathsCallback callback);
  void SetDeleteCallback(PathsCallback callback);

 private:
  void ShowMenu(const std::vector<std::string> &paths);
  void Activate(const std::string &path);

  GtkWidget *widget_ = nullptr;
  GtkWidget *path_label_ = nullptr;
  FileViewTreeModel model_;
  std::unique_ptr<FileViewTree> tree_;
  std::unique_ptr<FileViewList> list_;
  std::string path_;
  std::string home_;
  PathsCallback add_to_playlist_;
  PathsCallback copy_to_collection_;
  PathsCallback copy_to_device_;
  PathsCallback edit_tags_;
  PathsCallback delete_;
};

#endif
