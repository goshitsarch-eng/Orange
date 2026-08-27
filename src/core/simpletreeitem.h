#ifndef STRAWBERRY_SIMPLETREEITEM_H
#define STRAWBERRY_SIMPLETREEITEM_H

#include <memory>
#include <string>
#include <vector>

template <typename T>
class SimpleTreeItem {
 public:
  std::string key;
  T data;
  std::vector<std::unique_ptr<SimpleTreeItem<T>>> children;
};

#endif
