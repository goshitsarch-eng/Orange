#ifndef STRAWBERRY_FILEVIEWLIST_H
#define STRAWBERRY_FILEVIEWLIST_H

#include <functional>
#include <string>
#include <vector>

#include <gtk/gtk.h>

class FileViewList {
 public:
  using ActivateCallback = std::function<void(const std::string &)>;
  using MenuCallback = std::function<void(const std::vector<std::string> &)>;

  FileViewList();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void Reload(const std::vector<std::string> &paths);
  void SetActivateCallback(ActivateCallback callback);
  void SetMenuCallback(MenuCallback callback);
  std::vector<std::string> SelectedPaths() const;

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  ActivateCallback activate_;
  MenuCallback menu_;
};

#endif
