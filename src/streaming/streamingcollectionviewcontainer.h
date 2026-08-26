#ifndef STRAWBERRY_STREAMINGCOLLECTIONVIEWCONTAINER_H
#define STRAWBERRY_STREAMINGCOLLECTIONVIEWCONTAINER_H

#include "streaming/streamingcollectionview.h"

#include <memory>
#include <string>

class StreamingCollectionViewContainer {
 public:
  explicit StreamingCollectionViewContainer(const std::string &title);

  GtkWidget *widget() const { return view_->widget(); }
  StreamingCollectionView *view() const { return view_.get(); }

 private:
  std::unique_ptr<StreamingCollectionView> view_;
};

#endif
