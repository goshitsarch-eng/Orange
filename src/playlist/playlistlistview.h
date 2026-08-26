#ifndef STRAWBERRY_PLAYLISTLISTVIEW_H
#define STRAWBERRY_PLAYLISTLISTVIEW_H

#include "playlist/playlistlistdrop.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>
#include <vector>

class PlaylistListView {
 public:
  using ActivateCallback = std::function<void(const std::string &)>;
  using MenuCallback = std::function<void(const std::string &)>;
  using DropCallback = std::function<void(const std::string &, const std::string &)>;

  PlaylistListView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void Refresh(const std::vector<PlaylistListDrop::Row> &rows, const std::string &current);
  void SetActivateCallback(ActivateCallback callback);
  void SetMenuCallback(MenuCallback callback) { menu_ = std::move(callback); }
  void SetDropCallback(DropCallback callback) { drop_ = std::move(callback); }
  std::string SelectedName() const;

 private:
  void SetupRowDrop(GtkWidget *row, const std::string &name);

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  ActivateCallback activate_;
  MenuCallback menu_;
  DropCallback drop_;
};

#endif
