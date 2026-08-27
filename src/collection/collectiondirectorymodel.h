#ifndef STRAWBERRY_COLLECTIONDIRECTORYMODEL_H
#define STRAWBERRY_COLLECTIONDIRECTORYMODEL_H

#include "collection/collectiondirectory.h"

#include <string>
#include <vector>

class CollectionBackend;

class CollectionDirectoryModel {
 public:
  explicit CollectionDirectoryModel(CollectionBackend *backend = nullptr);

  void Reload();
  const std::vector<CollectionDirectory> &directories() const { return directories_; }
  int Count() const { return static_cast<int>(directories_.size()); }
  const CollectionDirectory *At(int index) const;

 private:
  CollectionBackend *backend_ = nullptr;
  std::vector<CollectionDirectory> directories_;
};

#endif
