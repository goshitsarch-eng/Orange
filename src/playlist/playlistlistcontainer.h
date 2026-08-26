#ifndef STRAWBERRY_PLAYLISTLISTCONTAINER_H
#define STRAWBERRY_PLAYLISTLISTCONTAINER_H

#include "playlist/playlistlistmodel.h"
#include "playlist/playlistlistsortfiltermodel.h"
#include "playlist/playlistlistview.h"
#include "playlist/playlistmanager.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>

class PlaylistListContainer {
 public:
  PlaylistListContainer();

  GtkWidget *widget() const { return widget_; }
  PlaylistListView *view() { return view_.get(); }
  PlaylistListModel *model() { return &model_; }
  void Reload(PlaylistManager *manager);
  void SetActivateCallback(const std::function<void(const std::string &)> &callback);

 private:
  GtkWidget *widget_ = nullptr;
  PlaylistListModel model_;
  PlaylistListSortFilterModel filter_;
  std::unique_ptr<PlaylistListView> view_;
};

#endif
