#include "collection/collectiondirectorymodel.h"

#include "collection/collectionbackend.h"

CollectionDirectoryModel::CollectionDirectoryModel(CollectionBackend *backend) : backend_(backend) { Reload(); }

void CollectionDirectoryModel::Reload() {
  directories_ = backend_ ? backend_->Directories() : std::vector<CollectionDirectory>{};
}

const CollectionDirectory *CollectionDirectoryModel::At(int index) const {
  if (index < 0 || index >= Count()) {
    return nullptr;
  }
  return &directories_[static_cast<size_t>(index)];
}
