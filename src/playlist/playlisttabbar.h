#ifndef STRAWBERRY_PLAYLISTTABBAR_H
#define STRAWBERRY_PLAYLISTTABBAR_H

#include "playlist/playlistmanager.h"
#include "widgets/favoritewidget.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

class PlaylistTabBar {
 public:
  using ChangedCallback = std::function<void(const std::string &)>;
  using FavoriteCallback = std::function<void(const std::string &, bool)>;

  PlaylistTabBar();

  GtkWidget *widget() const { return widget_; }
  void Refresh(PlaylistManager *manager);
  void SetChangedCallback(ChangedCallback callback);
  void SetFavoriteCallback(FavoriteCallback callback);

 private:
  GtkWidget *widget_ = nullptr;
  ChangedCallback changed_;
  FavoriteCallback favorite_;
  std::vector<std::unique_ptr<FavoriteWidget>> favorites_;
};

#endif
