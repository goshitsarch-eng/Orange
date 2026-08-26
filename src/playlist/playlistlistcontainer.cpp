#include "playlist/playlistlistcontainer.h"

PlaylistListContainer::PlaylistListContainer()
    : widget_(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)), filter_(&model_), view_(std::make_unique<PlaylistListView>()) {
  gtk_box_append(GTK_BOX(widget_), view_->widget());
}

void PlaylistListContainer::Reload(PlaylistManager *manager) {
  model_.Reload(manager);
  view_->Refresh(model_);
}

void PlaylistListContainer::SetActivateCallback(const std::function<void(const std::string &)> &callback) {
  view_->SetActivateCallback(callback);
}
