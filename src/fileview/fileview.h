#ifndef STRAWBERRY_FILEVIEW_H
#define STRAWBERRY_FILEVIEW_H

#include "fileview/fileviewhistory.h"
#include "fileview/fileviewlist.h"
#include "fileview/fileviewmenu.h"
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

  void SetPath(const std::string &path, bool record = true);
  void FileUp();
  void FileHome();
  void FileBack();
  void FileForward();
  void Reload();
  void SetShowHidden(bool show_hidden);
  void SetShowAllFiles(bool show_all);
  FileViewHistory *history() { return &history_; }
  void SetAddToPlaylistCallback(PathsCallback callback);
  void SetReplacePlaylistCallback(PathsCallback callback);
  void SetOpenInNewCallback(PathsCallback callback);
  void SetCopyToCollectionCallback(PathsCallback callback);
  void SetMoveToCollectionCallback(PathsCallback callback);
  void SetCopyToDeviceCallback(PathsCallback callback);
  void SetEditTagsCallback(PathsCallback callback);
  void SetDeleteCallback(PathsCallback callback);
  void SetShowInBrowserCallback(PathsCallback callback);

 private:
  void ShowMenu(const std::vector<std::string> &paths);
  void Activate(const std::string &path);
  void UpdateNavButtons();

  void PersistSettings();

  GtkWidget *widget_ = nullptr;
  GtkWidget *path_entry_ = nullptr;
  GtkWidget *back_ = nullptr;
  GtkWidget *forward_ = nullptr;
  GtkWidget *hidden_btn_ = nullptr;
  GtkWidget *all_files_btn_ = nullptr;
  FileViewTreeModel model_;
  std::unique_ptr<FileViewTree> tree_;
  std::unique_ptr<FileViewList> list_;
  FileViewHistory history_;
  std::string path_;
  std::string home_;
  PathsCallback add_to_playlist_;
  PathsCallback replace_playlist_;
  PathsCallback open_in_new_;
  PathsCallback copy_to_collection_;
  PathsCallback move_to_collection_;
  PathsCallback copy_to_device_;
  PathsCallback edit_tags_;
  PathsCallback delete_;
  PathsCallback show_in_browser_;
};

#endif
