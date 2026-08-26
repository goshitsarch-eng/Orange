#ifndef STRAWBERRY_SAVEDGROUPINGMANAGER_H
#define STRAWBERRY_SAVEDGROUPINGMANAGER_H

#include "collection/collectiongrouping.h"

#include <gtk/gtk.h>

#include <functional>

class SavedGroupingManager {
 public:
  static void Show(GtkWindow *parent, const std::function<void(const CollectionGrouping::Grouping &)> &callback);
};

#endif
