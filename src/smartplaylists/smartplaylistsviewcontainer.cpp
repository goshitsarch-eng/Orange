#include "smartplaylists/smartplaylistsviewcontainer.h"

SmartPlaylistsViewContainer::SmartPlaylistsViewContainer() {
  widget_ = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  view_ = std::make_unique<SmartPlaylistsView>();
  gtk_widget_set_vexpand(view_->widget(), TRUE);
  gtk_box_append(GTK_BOX(widget_), view_->widget());
  Reload();
}

void SmartPlaylistsViewContainer::Reload() {
  model_.Reload();
  view_->Reload(&model_);
}

void SmartPlaylistsViewContainer::SetActivateCallback(std::function<void(const SmartPlaylistsItem &)> callback) {
  view_->SetActivateCallback(std::move(callback));
}
