#ifndef STRAWBERRY_SMARTPLAYLISTSVIEWCONTAINER_H
#define STRAWBERRY_SMARTPLAYLISTSVIEWCONTAINER_H

#include "smartplaylists/smartplaylistsmodel.h"
#include "smartplaylists/smartplaylistsview.h"

#include <functional>
#include <memory>

#include <gtk/gtk.h>

class SmartPlaylistsViewContainer {
 public:
  SmartPlaylistsViewContainer();

  GtkWidget *widget() const { return widget_; }
  SmartPlaylistsModel *model() { return &model_; }
  SmartPlaylistsView *view() { return view_.get(); }
  void Reload();
  void SetActivateCallback(std::function<void(const SmartPlaylistsItem &)> callback);
  void SetDeleteCallback(std::function<void(const SmartPlaylistsItem &)> callback);

 private:
  GtkWidget *widget_ = nullptr;
  SmartPlaylistsModel model_;
  std::unique_ptr<SmartPlaylistsView> view_;
};

#endif
