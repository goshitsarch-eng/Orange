#include "streaming/streamingsongsview.h"

StreamingSongsView::StreamingSongsView(StreamingService *service)
    : service_(service), container_(std::make_unique<StreamingCollectionViewContainer>("Songs")) {
  container_->view()->SetRefreshCallback([this]() { Reload(); });
}

void StreamingSongsView::SetActivateCallback(ActivateCallback callback) { container_->view()->SetActivateCallback(std::move(callback)); }

void StreamingSongsView::Reload() {
  if (!service_) {
    return;
  }
  container_->view()->SetStatus("Loading songs…");
  service_->GetSongs([this](const SongList &songs) { container_->view()->SetSongs(songs); });
}
