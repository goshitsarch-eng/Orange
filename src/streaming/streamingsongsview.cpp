#include "streaming/streamingsongsview.h"

#include "streaming/streamingprogress.h"

StreamingSongsView::StreamingSongsView(StreamingService *service)
    : service_(service), container_(std::make_unique<StreamingCollectionViewContainer>("Songs")) {
  container_->view()->SetService(service_);
  container_->view()->SetRefreshCallback([this]() { Reload(); });
  container_->SetAbortCallback([this]() { AbortGetSongs(); });
  if (service_) {
    const auto alive = alive_;
    service_->SongsUpdateStatus.Connect([this, alive](const std::string &text) {
      if (alive && *alive) {
        container_->SetProgressStatus(text);
        container_->ShowProgress();
      }
    });
    service_->SongsProgressSetMaximum.Connect([this, alive](int maximum) {
      if (alive && *alive) {
        container_->SetProgressMaximum(maximum);
      }
    });
    service_->SongsUpdateProgress.Connect([this, alive](int value) {
      if (alive && *alive) {
        container_->SetProgress(value);
      }
    });
    service_->SongsFailed.Connect([this, alive](const std::string &error) {
      if (alive && *alive) {
        container_->ShowError(error);
      }
    });
  }
}

StreamingSongsView::~StreamingSongsView() {
  if (alive_) {
    *alive_ = false;
  }
}

void StreamingSongsView::SetActivateCallback(ActivateCallback callback) { container_->view()->SetActivateCallback(std::move(callback)); }

void StreamingSongsView::Reload() {
  if (!service_) {
    return;
  }
  if (StreamingProgress::ShouldShowBrowse(service_->show_progress(), true)) {
    container_->ShowProgress(StreamingProgress::ReceivingSongs());
    service_->StartSongsProgress();
  }
  container_->view()->SetStatus("Loading songs…");
  service_->GetSongs([this](const SongList &songs) {
    container_->HideProgressUnlessError();
    container_->view()->SetSongs(songs);
  });
}

void StreamingSongsView::AbortGetSongs() {
  if (service_) {
    service_->ResetSongsRequest();
  }
  container_->HideProgress();
}
