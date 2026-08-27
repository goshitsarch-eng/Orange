#ifndef STRAWBERRY_FILEVIEWLIST_H
#define STRAWBERRY_FILEVIEWLIST_H

#include "fileview/fileviewkeyboard.h"

#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class FileViewList {
 public:
  using ActivateCallback = std::function<void(const std::string &)>;
  using EnqueueCallback = std::function<void(const std::vector<std::string> &)>;
  using MenuCallback = std::function<void(const std::vector<std::string> &)>;
  using NavigateCallback = std::function<void(FileViewKeyboard::Action)>;

  FileViewList();
  ~FileViewList();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void Reload(const std::vector<std::string> &paths);
  void SetActivateCallback(ActivateCallback callback);
  void SetDoubleClickCallback(ActivateCallback callback);
  void SetEnqueueCallback(EnqueueCallback callback);
  void SetMenuCallback(MenuCallback callback);
  void HandlePress(guint button, gint n_press, double x, double y, GdkModifierType state);
  void SetNavigateCallback(NavigateCallback callback);
  std::vector<std::string> SelectedPaths() const;

 private:
  void SetupRowDrag(GtkWidget *row, const std::string &path);
  gboolean OnKeyPressed(guint keyval, GdkModifierType mods);
  void ResetTypeAhead();
  std::vector<std::string> Labels() const;

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  ActivateCallback activate_;
  ActivateCallback double_click_;
  EnqueueCallback enqueue_;
  MenuCallback menu_;
  NavigateCallback navigate_;
  std::string typeahead_;
  guint typeahead_timeout_ = 0;
};

#endif
