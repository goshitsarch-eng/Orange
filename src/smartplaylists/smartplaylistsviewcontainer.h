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
  void ApplyLook();
  void RefreshOnShow();
  void SetActivateCallback(std::function<void(const SmartPlaylistsItem &)> callback);
  void SetDeleteCallback(std::function<void(const SmartPlaylistsItem &)> callback);
  void SetActionCallback(std::function<void(const SmartPlaylistsItem &, SmartPlaylistsAction)> callback);
  void SetSongsCallback(SmartPlaylistsView::SongsCallback callback);

 private:
  void EmitSelected(SmartPlaylistsAction action);
  void UpdateButtons();

  GtkWidget *widget_ = nullptr;
  GtkWidget *add_button_ = nullptr;
  GtkWidget *edit_button_ = nullptr;
  GtkWidget *delete_button_ = nullptr;
  GtkWidget *restore_button_ = nullptr;
  SmartPlaylistsModel model_;
  std::unique_ptr<SmartPlaylistsView> view_;
};

#endif
