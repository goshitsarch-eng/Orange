#ifndef STRAWBERRY_SMARTPLAYLISTSVIEW_H
#define STRAWBERRY_SMARTPLAYLISTSVIEW_H

#include "smartplaylists/smartplaylistsmodel.h"

#include <functional>
#include <string>

#include <gtk/gtk.h>

class SmartPlaylistsView {
 public:
  SmartPlaylistsView();

  GtkWidget *widget() const { return widget_; }
  void Reload(SmartPlaylistsModel *model);
  void SetActivateCallback(std::function<void(const SmartPlaylistsItem &)> callback) { activate_ = std::move(callback); }
  void SetDeleteCallback(std::function<void(const SmartPlaylistsItem &)> callback) { delete_ = std::move(callback); }

 private:
  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::function<void(const SmartPlaylistsItem &)> activate_;
  std::function<void(const SmartPlaylistsItem &)> delete_;
};

#endif
