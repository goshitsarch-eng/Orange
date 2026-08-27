#ifndef STRAWBERRY_SIMPLETREEMODEL_H
#define STRAWBERRY_SIMPLETREEMODEL_H

#include "core/simpletreeitem.h"

template <typename T>
class SimpleTreeModel {
 public:
  SimpleTreeItem<T> *root() { return &root_; }

 private:
  SimpleTreeItem<T> root_;
};

#endif
