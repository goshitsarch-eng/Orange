#ifndef STRAWBERRY_PLAYLISTLISTVIEW_H
#define STRAWBERRY_PLAYLISTLISTVIEW_H

#include "playlist/playlistlistmodel.h"

#include <gtk/gtk.h>

#include <functional>
#include <string>

class PlaylistListView {
 public:
  using ActivateCallback = std::function<void(const std::string &)>;

  PlaylistListView();

  GtkWidget *widget() const { return widget_; }
  GtkWidget *list() const { return list_; }
  void Refresh(const PlaylistListModel &model);
  void SetActivateCallback(ActivateCallback callback);

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  ActivateCallback activate_;
};

#endif
