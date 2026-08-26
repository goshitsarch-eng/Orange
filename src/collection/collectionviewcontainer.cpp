#include "collection/collectionviewcontainer.h"

CollectionViewContainer::CollectionViewContainer()
    : widget_(gtk_box_new(GTK_ORIENTATION_VERTICAL, 0)),
      filter_widget_(std::make_unique<CollectionFilterWidget>()),
      view_(std::make_unique<CollectionView>()) {
  gtk_box_append(GTK_BOX(widget_), filter_widget_->widget());
  gtk_box_append(GTK_BOX(widget_), view_->widget());
}
