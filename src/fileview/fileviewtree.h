#ifndef STRAWBERRY_FILEVIEWTREE_H
#define STRAWBERRY_FILEVIEWTREE_H

#include "fileview/fileviewtreemodel.h"

#include <functional>
#include <string>

#include <gtk/gtk.h>

class FileViewTree {
 public:
  using ActivateCallback = std::function<void(const std::string &)>;
  using MenuCallback = std::function<void(const std::string &)>;

  FileViewTree();

  GtkWidget *widget() const { return widget_; }
  void Reload(FileViewTreeModel *model);
  void SetActivateCallback(ActivateCallback callback);
  void SetMenuCallback(MenuCallback callback);

 private:
  void AppendItem(GtkWidget *parent, FileViewTreeItem *item);

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  ActivateCallback activate_;
  MenuCallback menu_;
};

#endif
