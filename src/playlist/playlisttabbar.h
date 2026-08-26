#ifndef STRAWBERRY_PLAYLISTTABBAR_H
#define STRAWBERRY_PLAYLISTTABBAR_H

#include "playlist/playlistlistlook.h"
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
  using CloseCallback = std::function<void(int)>;

  PlaylistTabBar();

  GtkWidget *widget() const { return widget_; }
  void Refresh(PlaylistManager *manager, const std::string &active_name = {},
               PlaylistListLook::Playback playback = PlaylistListLook::Playback::Stopped);
  void SetChangedCallback(ChangedCallback callback);
  void SetFavoriteCallback(FavoriteCallback callback);
  void SetCloseCallback(CloseCallback callback) { close_ = std::move(callback); }

 private:
  GtkWidget *widget_ = nullptr;
  ChangedCallback changed_;
  FavoriteCallback favorite_;
  CloseCallback close_;
  std::vector<std::unique_ptr<FavoriteWidget>> favorites_;
};

#endif
