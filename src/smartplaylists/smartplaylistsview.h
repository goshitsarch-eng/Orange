#ifndef STRAWBERRY_SMARTPLAYLISTSVIEW_H
#define STRAWBERRY_SMARTPLAYLISTSVIEW_H

#include "smartplaylists/smartplaylistsmodel.h"

#include <functional>
#include <string>

#include <gtk/gtk.h>

enum class SmartPlaylistsAction { Activate, Append, Replace, Queue, Edit, Delete };

class SmartPlaylistsView {
 public:
  SmartPlaylistsView();

  GtkWidget *widget() const { return widget_; }
  void Reload(SmartPlaylistsModel *model);
  void SetActivateCallback(std::function<void(const SmartPlaylistsItem &)> callback) { activate_ = std::move(callback); }
  void SetDeleteCallback(std::function<void(const SmartPlaylistsItem &)> callback) { delete_ = std::move(callback); }
  void SetActionCallback(std::function<void(const SmartPlaylistsItem &, SmartPlaylistsAction)> callback) {
    action_ = std::move(callback);
  }

 private:
  void Emit(const SmartPlaylistsItem &item, SmartPlaylistsAction action);
  void ShowMenu(GtkWidget *relative, const SmartPlaylistsItem &item);

  GtkWidget *widget_ = nullptr;
  GtkWidget *list_ = nullptr;
  std::function<void(const SmartPlaylistsItem &)> activate_;
  std::function<void(const SmartPlaylistsItem &)> delete_;
  std::function<void(const SmartPlaylistsItem &, SmartPlaylistsAction)> action_;
};

#endif
