#ifndef STRAWBERRY_STREAMINGSONGSVIEW_H
#define STRAWBERRY_STREAMINGSONGSVIEW_H

#include "streaming/streamingcollectionviewcontainer.h"
#include "streaming/streamingservices.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>

class StreamingSongsView {
 public:
  using ActivateCallback = std::function<void(const Song &)>;

  explicit StreamingSongsView(StreamingService *service);
  GtkWidget *widget() const { return container_->widget(); }
  void SetActivateCallback(ActivateCallback callback);
  void Reload();
  StreamingCollectionView *view() const { return container_->view(); }

 private:
  StreamingService *service_ = nullptr;
  std::unique_ptr<StreamingCollectionViewContainer> container_;
};

#endif
