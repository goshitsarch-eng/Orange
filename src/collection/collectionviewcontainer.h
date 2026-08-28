#ifndef STRAWBERRY_COLLECTIONVIEWCONTAINER_H
#define STRAWBERRY_COLLECTIONVIEWCONTAINER_H

#include "collection/collectionfilterwidget.h"
#include "collection/collectionview.h"

#include <gtk/gtk.h>

#include <functional>
#include <memory>

class CollectionViewContainer {
 public:
  CollectionViewContainer();

  GtkWidget *widget() const { return widget_; }
  CollectionView *view() const { return view_.get(); }
  CollectionFilterWidget *filter_widget() const { return filter_widget_.get(); }
  void ApplyLook();

 private:
  GtkWidget *widget_ = nullptr;
  std::unique_ptr<CollectionFilterWidget> filter_widget_;
  std::unique_ptr<CollectionView> view_;
};

#endif
