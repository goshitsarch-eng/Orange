#ifndef STRAWBERRY_FILEVIEWTREE_H
#define STRAWBERRY_FILEVIEWTREE_H

#include "fileview/fileviewtreemodel.h"

#include <functional>
#include <string>

#include <gtk/gtk.h>

class FileViewTree {
 public:
  using ActivateCallback = std::function<void(const std::string &)>;
  using EnqueueCallback = std::function<void(const std::vector<std::string> &)>;
  using MenuCallback = std::function<void(const std::string &)>;

  FileViewTree();

  GtkWidget *widget() const { return widget_; }
  void Reload(FileViewTreeModel *model);
  void SetActivateCallback(ActivateCallback callback);
  void SetDoubleClickCallback(ActivateCallback callback);
  void SetEnqueueCallback(EnqueueCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state);
  std::string SelectedPath() const;

 private:
  void AppendItem(GtkWidget *parent, FileViewTreeItem *item, int depth);
  void SetupRowDrag(GtkWidget *row, const std::string &path);
  gboolean OnKeyPressed(guint keyval, GdkModifierType state);

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  ActivateCallback activate_;
  ActivateCallback double_click_;
  EnqueueCallback enqueue_;
  MenuCallback menu_;
};

#endif
