#ifndef STRAWBERRY_FAVORITEWIDGET_H
#define STRAWBERRY_FAVORITEWIDGET_H

#include <gtk/gtk.h>

#include <functional>

class FavoriteWidget {
 public:
  using ChangedCallback = std::function<void(int, bool)>;

  explicit FavoriteWidget(int tab_index = -1, bool favorite = false);

  GtkWidget *widget() const { return widget_; }
  bool IsFavorite() const { return favorite_; }
  void SetFavorite(bool favorite);
  void SetChangedCallback(ChangedCallback callback);
  int tab_index() const { return tab_index_; }

  static const char *TooltipText() {
    return "Double-click here to favorite this playlist so it will be saved and remain accessible through the \"Playlists\" panel on the left side bar";
  }

 private:
  void Refresh();
  void Toggle();

  GtkWidget *widget_ = nullptr;
  int tab_index_ = -1;
  bool favorite_ = false;
  ChangedCallback changed_;
};

#endif
