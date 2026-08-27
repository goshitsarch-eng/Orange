#ifndef STRAWBERRY_GROUPBYDIALOG_H
#define STRAWBERRY_GROUPBYDIALOG_H

#include "collection/collectiongrouping.h"

#include <gtk/gtk.h>

#include <functional>

class GroupByDialog {
 public:
  static void Show(GtkWindow *parent, const CollectionGrouping::Grouping &current,
                   const std::function<void(const CollectionGrouping::Grouping &)> &callback);
};

#endif
